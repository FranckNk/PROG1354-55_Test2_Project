/*

TITRE          : Projet PROG1354 & 1355
AUTEUR         : Franck Nkeubou Awougang
DATE           : 04/04/2022
DESCRIPTION    : Programme pour détecter la température dans une machine et consulter via une page web
									ou un site web du même réseau.
VERSION        : 0.0.1

*/



#include <Arduino.h>

#include <Wire.h>
// Librairies pour l'écran OLED.
#include <Adafruit_GFX.h> // Modification du code de la librairie pour afficher les informations (IP, msg).
#include <Adafruit_SSD1306.h>
#include <SPI.h>
// Librairie pour le BMP280. Modification du code de test.
#include <Adafruit_BMP280.h>

// Import required libraries for using Server.
#include <WiFi.h>
// #include <Hash.h>
// #include <ESPAsyncTCP.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
// Import OTA update librairies required.
#include <AsyncElegantOTA.h>
#include "RTClib.h"
#include "Timer.h"

#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

#define SEALEVELPRESSURE_HPA (1013.25)


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_BMP280 bmp; // I2C
//Adafruit_BMP280 bmp(BMP_CS); // hardware SPI
//Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO,  BMP_SCK);



// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Replace with your network credentials
const char* ssid = "FibreOP532";
const char* password = "9PXPE66PM6XM55M8";

/*
const char* ssid = "IDO-OBJECTS";
const char* password = "42Bidules!";
*/


const char* http_username = "admin";
const char* http_password = "admin";

RTC_DS3231 rtc;
DateTime TempsActuel;
String DateActuelle;
String ActuelTime;

char Days[7][12] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// current temperature & humidity, updated in loop()
float Temperature = 0.0;
float Humidite = 0.0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time BMP280 was updated

// Updates BMP280 readings every 10 seconds
const long Interval = 8000;  
Timer TempsPause;
Timer TempsPauseValues;

bool StateLED = false;
uint8_t PinLED = 2;

// Déclaration des fonctions. 
void BlinkLED();

// Page Web root/
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
		<head>
				<meta charset="UTF-8">
				<meta http-equiv="X-UA-Compatible" content="IE=edge">
				<meta name="viewport" content="width=device-width, initial-scale=1.0">

				<script src="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/js/bootstrap.bundle.min.js" integrity="sha384-ka7Sk0Gln4gmtz2MlQnikT1wXgYsOg+OMhuP+IlRH9sENBO0LRn5q+8nbTov4+1p" crossorigin="anonymous"></script>
				<link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet" crossorigin="anonymous">
				<link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css"  crossorigin="anonymous">

				<title>Projet PROG13554</title>
				
				<style>
						.row{
								text-align: center;
								height: 100px;
								margin: auto;
								background-color: gray;

						}
						.col-md-4, .col-md-9{
								padding-top: 40px;
								font-size: 25px;
								font-weight: bold;
								background-color: rgb(234, 234, 234);
						}
						.col-md-3{
								color: rgb(3, 101, 161);
								padding-top: 20px;
								font-size: 50px;
								flex: auto;
								background-color: rgb(234, 234, 234);
						}
						.headenn{
								font-size: xx-large;
								text-align: center;
								padding: 5rem;
								height: 80px;
								font-weight: bold;
								box-sizing: unset;
								color: white;
								background-color: black;
						}
				</style>
		</head>
		<body>
				<div class="container" style="width: 70rem; padding-left: 0px; padding-right: 0px; margin-top: 100px; height: 600px; box-shadow: 10px 10px 10px 10px rgb(181, 181, 181);">
						<div class="row headenn">
								<div class="col-xs-12">ESP32 Weather <br>Station</div>
						</div>
						<div class="row justify-content-between">
								<div class="col-md-3"><i class="fas fa-temperature-high"></i></div>
								<div class="col-md-4 center">Temperature</div>
								<div class="col-md-3"><span id="temperature">%TEMPERATURE%</span>°<sup style="font-size: 20px;">C</sup></div>
						</div>
						<div class="row justify-content-between">
								<div class="col-md-3"><i class="fas fa-mountain"></i></div>
								<div class="col-md-4 center">Pressure</div>
								<div class="col-md-3"><span id="humidity">%HUMIDITY%</span><sup style="font-size: 20px;">*</sup></div>
						</div>
						<div class="row justify-content-between">
								<div class="col-md-3"><i class="fas fa-stopwatch"></i></div>
								<div class="col-md-4 center"><span id="date">%DATE%</span></div>
								<div class="col-md-3"><span id="time">%TIME%</span></div>
						</div>
				</div>
				<br><br><br>
				<button type="button" href = "/update" onclick="Update()" class="btn btn-primary ">Update Firmware</button>
				<a></a>
		</body>
		<script>
				
				function Update() {
					var xhr = new XMLHttpRequest();
					xhr.open("GET", "/logout", true);
					xhr.send();
					setTimeout(function(){ window.open("/update","_self"); }, 500);
				}

				setInterval(function ( ) {
					var xhttp = new XMLHttpRequest();
					xhttp.onreadystatechange = function() {
						if (this.readyState == 4 && this.status == 200) {
							document.getElementById("temperature").innerHTML = this.responseText;
						}
					};
					xhttp.open("GET", "/temperature", true);
					xhttp.send();
				}, 10000 ) ;
				
				setInterval(function ( ) {
					var xhttp = new XMLHttpRequest();
					xhttp.onreadystatechange = function() {
						if (this.readyState == 4 && this.status == 200) {
							document.getElementById("humidity").innerHTML = this.responseText;
						}
					};
					xhttp.open("GET", "/humidity", true);
					xhttp.send();
				}, 10000 ) ;

			 setInterval(function ( ) {
					var xhttp = new XMLHttpRequest();
					xhttp.onreadystatechange = function() {
						if (this.readyState == 4 && this.status == 200) {
							document.getElementById("date").innerHTML = this.responseText;
						}
					};
					xhttp.open("GET", "/date", true);
					xhttp.send();
				}, 10000 ) ;

				setInterval(function ( ) {
					var xhttp = new XMLHttpRequest();
					xhttp.onreadystatechange = function() {
						if (this.readyState == 4 && this.status == 200) {
							document.getElementById("time").innerHTML = this.responseText;
						}
					};
					xhttp.open("GET", "/time", true);
					xhttp.send();
				}, 1000 ) ;
				</script>
</html>)rawliteral";

// Replaces placeholder with BMP280 values
String processor(const String& var){
	//Serial.println(var);
	if(var == "TEMPERATURE"){
		return String(Temperature);
	}
	else if(var == "HUMIDITY"){
		return String(Humidite);
	}
	else if(var == "DATE")
		return DateActuelle;
	else if(var == "TIME")
		return ActuelTime;
	return String();
}

void setup() {
	Serial.begin(9600);

	// Lancement de l'écran OLED.
	if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
		Serial.println(F("SSD1306 allocation failed"));
		for(;;);
	}

	// while ( !Serial ) delay(100);   // wait for native usb
	WiFi.begin(ssid, password);

	display.clearDisplay();
	display.setTextSize(1);
	display.setTextColor(INVERSE);
	display.setCursor(0, 0);
	display.println("Connecting to WiFi...\n");
	Serial.println("Connecting to WiFi.");
	
	while (WiFi.status() != WL_CONNECTED) {
		delay(1000);
		Serial.println(".");
	}
	display.println("Connected.\n");
	// Serial.println(F("BMP280 test"));
	unsigned status;
	//status = bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);
	status = bmp.begin(0x76);
	if (!status) {
		display.setTextSize(1);
		display.println(F("Could not find a valid BMP280 sensor, check wiring or "
											"try a different address!"));
	}

	/* Default settings from datasheet. */
	bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
									Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
									Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
									Adafruit_BMP280::FILTER_X16,      /* Filtering. */
									Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
	if (! rtc.begin()) {
		display.setTextSize(1);
		display.println("Couldn't find RTC");
		Serial.flush();
		while (1) delay(10);
	}
	if (rtc.lostPower()) {
		Serial.println("RTC lost power, let's set the time!");
		// When time needs to be set on a new device, or after a power loss, the
		// following line sets the RTC to the date & time this sketch was compiled
		rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
		// This line sets the RTC with an explicit date & time, for example to set
		// January 21, 2014 at 3am you would call:
		// rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
		}

	 // Print ESP8266 Local IP Address
	display.setTextSize(1);
	display.println("IP Address : \n");
	display.println(WiFi.localIP());
	display.display(); 
	Serial.println(WiFi.localIP());

	// Route for root / web page
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send_P(200, "text/html", index_html, processor);
	});
	// configuration du serveur pour chaque type de requêtes.
	server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send_P(200, "text/plain", String(Temperature).c_str());
	});
	server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
	 request->send_P(200, "text/plain", String(Humidite).c_str());
		
	});

	server.on("/date", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send_P(200, "text/plain", DateActuelle.c_str());
	});
	
	// Disconnect the user.
	server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(401);
	});

	server.on("/time", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send_P(200, "text/plain", ActuelTime.c_str());
	});

	// Start server with credentials informations.
	AsyncElegantOTA.begin(&server, http_username, http_password);    // Start ElegantOTA
	//Start serveur
	server.begin();
	Serial.println("HTTP server started");
	TempsPause.startTimer(Interval/1000);
}

void loop() {
	if (TempsPause.isTimerReady()) // Si on a attendu près d'une seconde, on peut recalculer le temps.
	{
				
		TempsActuel = rtc.now();
		// On construit la chaine pour la date.
		DateActuelle = "" + String(TempsActuel.year(), DEC) + "/" + String(TempsActuel.month(), DEC) + "/" + 
									String(TempsActuel.day(), DEC) + "(" + String(Days[TempsActuel.dayOfTheWeek()]) + ")";
		// On construit la chaine pour l'heure.
		ActuelTime = "" + String(TempsActuel.hour(), DEC) + ":" + String(TempsActuel.minute(), DEC) + ":" + 
									String(TempsActuel.second(), DEC);

		if (TempsPauseValues.isTimerReady()) {
			float newT = bmp.readTemperature();
			if (isnan(newT)) {
				// Serial.println("Failed to read from BMP280 sensor!");
			}
			else {
				Temperature = newT;
				// Serial.print(F("Temperature = "));
				// Serial.print(t);
				// Serial.println(" *C");
			}
			// Read Humidity
			float newH = bmp.readPressure();
			// if humidity read failed, don't change h value 
			if (isnan(newH)) {
				// Serial.println("Failed to read from BMP280 sensor!");
			}
			else {
				Humidite = newH;
				// Serial.print(F("Pressure = "));
				// Serial.print(h);
				// Serial.println(" Pa");
			}
			TempsPauseValues.startTimer(Interval);
		}
		// Serial.print(F("The actual date and time is : "));
		// Serial.println(DateActuelle);
		TempsPause.startTimer(Interval/10);
		BlinkLED();
	}
}

void BlinkLED(){
	StateLED = !StateLED;
	digitalWrite(PinLED, StateLED);
}
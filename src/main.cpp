#include <Arduino.h>
/***************************************************************************
  This is a library for the BMP280 humidity, temperature & pressure sensor

  Designed specifically to work with the Adafruit BMP280 Breakout
  ----> http://www.adafruit.com/products/2651

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>


/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com/esp8266-BMP28011BMP28022-temperature-and-humidity-web-server-with-arduino-ide/
*********/

// Import required libraries
#include <WiFi.h>
// #include <Hash.h>
// #include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "RTClib.h"
#include "Timer.h"

#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

Adafruit_BMP280 bmp; // I2C
//Adafruit_BMP280 bmp(BMP_CS); // hardware SPI
//Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO,  BMP_SCK);

// Replace with your network credentials
/*
const char* ssid = "FibreOP532";
const char* password = "9PXPE66PM6XM55M8";
*/

const char* ssid = "IDO-OBJECTS";
const char* password = "42Bidules!";

RTC_DS3231 rtc;
DateTime TempsActuel;
String DateActuelle;

char Days[7][12] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time BMP280 was updated

// Updates BMP280 readings every 10 seconds
const long interval = 10000;  
Timer TempsPause;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .BMP280-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>ESP8266 BMP280 Server</h2>
  <p>
    <i class="fa-solid fa-clock-eight" style="color:#00add6;"></i> 
    <span class="BMP280-labels"></span>
    <span id="date">%DATE%</span>
  </p>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="BMP280-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="BMP280-labels">Pressure</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">%</sup>
  </p>
</body>
<script>
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
}, 1000 ) ;
</script>
</html>)rawliteral";

// Replaces placeholder with BMP280 values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return String(t);
  }
  else if(var == "HUMIDITY"){
    return String(h);
  }
  else if(var == "DATE")
    return DateActuelle;
  return String();
}

void setup() {
  Serial.begin(9600);
  while ( !Serial ) delay(100);   // wait for native usb
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }
  Serial.println(F("BMP280 test"));
  unsigned status;
  //status = bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);
  status = bmp.begin(0x76);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(),16);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
  if (! rtc.begin()) {
		Serial.println("Couldn't find RTC");
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
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(t).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(h).c_str());
    
  });
  server.on("/date", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", DateActuelle.c_str());
  });

  // Start server
  server.begin();
  TempsPause.startTimer(1000);
}

void loop() {
    if (TempsPause.isTimerReady())
    {
         
      TempsActuel = rtc.now();
      DateActuelle = "Date : " + String(TempsActuel.year(), DEC) + "/" + String(TempsActuel.month(), DEC) + "/" + 
                    String(TempsActuel.day(), DEC) + "(" + String(Days[TempsActuel.dayOfTheWeek()]) + ")   Heure : " +
                    String(TempsActuel.hour(), DEC) + ":" + String(TempsActuel.minute(), DEC) + ":" + 
                    String(TempsActuel.second(), DEC);
      unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      // save the last time you updated the BMP280 values
      previousMillis = currentMillis;
      // Read temperature as Celsius (the default)
      float newT = bmp.readTemperature();
      if (isnan(newT)) {
        // Serial.println("Failed to read from BMP280 sensor!");
      }
      else {
        t = newT;
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
        h = newH;
        // Serial.print(F("Pressure = "));
        // Serial.print(h);
        // Serial.println(" Pa");
      }
    }
    // Serial.print(F("The actual date and time is : "));
    // Serial.println(DateActuelle);
    TempsPause.startTimer(1000);
    }
}
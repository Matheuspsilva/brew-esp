#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <Hash.h>
#include <FS.h>
#include <ESPAsyncWebServer.h>
#include <OneWire.h>
#include <Wire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <LiquidCrystal_I2C.h>

#define MAX_RECIPE_FILE_SIZE 2024
#define RECIPE_FILE "/config/config.json"
// LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x0 for a 16 chars and 2 line display


// FS *_fs;
String Quantidade_rampas;
String Temperatura_1;
String Tempo_1;
String time_array[5];
String temperature_array[5];
String lastTemperature;
boolean brew_started = 0;
boolean init_clock = 0;
boolean flag = 1;
boolean preRamp = 1;
unsigned long brew_init_time = 0;
unsigned long ramp_start_time = 0;
int active_stage = 0;

// Interval between sensor readings.
unsigned long previousMillis = 0;     
const long interval = 5000; 
//Network Credentials
const char* ssid = "NewBrewSystem";
const char* password = "12345678";

AsyncWebServer server(80);

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

boolean loadRecipeSettings()
{
    File configFile = SPIFFS.open(RECIPE_FILE, "r");
    
    if (configFile)
    {
      Serial.println("Arquivo lido com sucesso");
        size_t size = configFile.size();
        if (size <= MAX_RECIPE_FILE_SIZE)
        {
            DynamicJsonDocument _activeStatusJsonDocument = DynamicJsonDocument(MAX_RECIPE_FILE_SIZE);
            DeserializationError error = deserializeJson(_activeStatusJsonDocument, configFile);
            if (error == DeserializationError::Ok && _activeStatusJsonDocument.is<JsonObject>())
            {
              JsonObject _activeStatus = _activeStatusJsonDocument.as<JsonObject>();
              Quantidade_rampas = _activeStatus["quantidade_rampas"].as<String>();

              for (size_t i = 0; i < Quantidade_rampas.toInt(); i++)
              {
                temperature_array[i] = _activeStatus["temperatura_" + String(i+1)].as<String>();
                time_array[i] = _activeStatus["tempo_" + String(i+1)].as<String>();
                Serial.println("temperatura_" + String(i) + " = " + String(temperature_array[i]) + " tempo_" + String(i) + " = " + String(time_array[i]));
              }
                configFile.close();
            }
        }
        configFile.close();
    }
    return true;
}

// GPIO where the output is connected to
const int output = 2;
// const int output = 13;
// GPIO where the DS18B20 is connected to
const int oneWireBus = 12;   

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

 String readDSTemperatureC() {
    // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
    sensors.requestTemperatures(); 
    float tempC = sensors.getTempCByIndex(0);

    if(tempC == -127.00) {
      Serial.println("Failed to read from DS18B20 sensor");
      return "--";
    } else {
      Serial.print("Temperature Celsius: ");
      Serial.println(tempC); 
    }
    return String(tempC);
  }
void setup(){

  //LCD
  lcd.init();// initialize the lcd 

  lcd.backlight();
  lcd.setCursor(1, 0);
  lcd.print("WELCOME");
  lcd.setCursor(1, 1);
  lcd.print("BREWESP");

  // Starts Serial Monitor with 9600 baud rate
  Serial.begin(9600);

  // Starts DS18B20 Sensor
  sensors.begin();

  //  Start SPIFFS read
  if(!SPIFFS.begin()){
     Serial.println("An Error has occurred while mounting SPIFFS");
     return;
  }

  //Load recipe config
  loadRecipeSettings();
  
  Serial.println("Configuring access point...");

  // Start access point 
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  // IP Address of our access point
  IPAddress ip_address = WiFi.softAPIP();
 
  // Start web server
  server.begin();
  Serial.print("AP IP address: ");
  Serial.println(ip_address);

  pinMode(output, OUTPUT);
  digitalWrite(output, LOW);
  
  // Send web page to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });

    // Send a GET request to <IP>/get?message=<message>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String message;
      if (request->hasParam("brew_started")) {
          brew_started = request->getParam("brew_started")->value();
          brew_init_time = millis();
          message = "Brew started";
      } else {
          message = "Brew not started";
      }

      // request->send(200, "text/plain", "Hello, GET: " + message);
      request->redirect("/");

  });

  server.on("/css/app.b7bec5aa.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/css/app.b7bec5aa.css", "text/css");
  });

  server.on("/css/chunk-vendors.367538c2.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/css/chunk-vendors.367538c2.css", "text/css");
  });

  server.on("/js/app.fdb535df.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/js/app.fdb535df.js");
  });

  server.on("/js/chunk-vendors.334450e7.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/js/chunk-vendors.334450e7.js");
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/favicon.ico");
  });

  server.on("/img/logo.2a84def0.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/img/logo.2a84def0.png");
  });

  //  Read Celsius Temperature 
  server.on("/temperaturec", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDSTemperatureC().c_str());
  });
}

void loop(){
  // Starts if
  if(brew_started){
    while(flag){
        Serial.println("Brew started");
        lcd.clear();
        lcd.print("Brew started");
        flag = 0;
    }
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      sensors.requestTemperatures();
      // Temperature in Celsius degrees 
      float temperature = sensors.getTempCByIndex(0);

      lcd.clear();
      lcd.setCursor (0,0); //character zero, line 1
      lcd.print("Temperatura: "); // print text  
      lcd.setCursor (14,0); //character zero, line 1
      lcd.print(temperature);
      
      lcd.setCursor (0,1); //character 4, line 2
      lcd.print("Etapa: "); // print text   

      lcd.setCursor (0,2); //character 0, line 3
      lcd.print("Tempo: "); // print text 

      int seconds = (currentMillis - brew_init_time) / 1000;
      // int seconds = currentMillis / 1000;
      int minutes = seconds / 60;
      seconds %= 60;
      lcd.setCursor (9,2); //character 0, line 3
      lcd.print(minutes);
      lcd.print(':');
      if (seconds < 10) {
        lcd.print('0');
      }
      lcd.print(seconds);
      
      //Se estiver em pré-rampa, acionar a resistência até chegar ao threshold
      if(preRamp){
        Serial.println("Pré rampa: " + String(active_stage + 1));
        lcd.setCursor (7,1); //character 4, line 2
        lcd.print("PRE RAMPA "); // print text 
        lcd.print(active_stage + 1); // print text 
        
        digitalWrite(output, HIGH);
        if(temperature >= (temperature_array[active_stage].toFloat())){
          init_clock = 0;
          preRamp = 0;
          Serial.println("Iniciando rampa: " + String(active_stage + 1));
          lcd.print("RAMPA "); // print text 
          lcd.setCursor (9,1); //character 4, line 2
          lcd.print(active_stage); // print text 
          //Se a temperatura ambiente for maior ou igual a temperatura alvo e se a rampa não foi iniciada, a rampa inicia
          while (init_clock == 0)
          {
              ramp_start_time = millis();
              Serial.println("ramp_start_time: " + String(ramp_start_time));
              init_clock = 1;
          }
        }
      }else{
        // Se Horário atual >= Horário do fim da rampa
        if(currentMillis >= ((time_array[active_stage].toFloat() * 1000) + ramp_start_time)){
          active_stage ++;
          preRamp = 1;
        }else if(temperature >= (temperature_array[active_stage].toFloat() + 1)){
          // Check if temperature is above threshold and if it needs to trigger output
          String message = String("Temperature above threshold(") + String(temperature_array[active_stage].toFloat()) + ("). Current temperature: ") + 
                    String(temperature) + String("C");
          Serial.println(message);
          //Turn off resistence
          Serial.println("Resistência desativada");
          digitalWrite(output, LOW);
        }else if(temperature < (temperature_array[active_stage].toFloat() - 1)){
          // Check if temperature is below threshold and if it needs to trigger output
        String message = String("Temperature below threshold(") + String(temperature_array[active_stage].toFloat()) + ("). Current temperature: ") + 
                                String(temperature) + String("C");
          Serial.println(message);
          //Turn on resistence
          Serial.println("Resistência ativada");
          digitalWrite(output, HIGH);
        }
        if(active_stage == (Quantidade_rampas.toInt() - 1)){
          brew_started = 0;
          Serial.println("Brassagem finalizada");
          Serial.println("Resistência desativada");
          digitalWrite(output, LOW);
        }
      }

    }
  }
}
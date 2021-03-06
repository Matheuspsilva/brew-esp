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
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x0 for a 16 chars and 2 line display


// FS *_fs;
String Quantidade_rampas;
String Temperatura_1;
String Tempo_1;
String time_array[5];
String temperature_array[5];
String lastTemperature;
String recipeName;
boolean brew_started = 0;
boolean init_clock = 0;
boolean display_brew_msg = 1;
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
WiFiServer server1(81);

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void showWelcome(){

  lcd.init();// initialize the lcd 
  lcd.backlight();
  lcd.setCursor(1, 0);
  lcd.print("WELCOME");
  lcd.setCursor(1, 1);
  lcd.print("ESPBREW");
}
void showTemperature(float temperature){

  lcd.clear();
  lcd.setCursor (0,0); //character zero, line 1
  lcd.print("Temperatura: "); // print text  
  lcd.setCursor (14,0); //character zero, line 1
  lcd.print(temperature);
}

void showActiveStage(int active_stage, boolean preRamp){

  lcd.setCursor (0,1); //character 4, line 2
  if(preRamp){
    lcd.print("PRE-RAMPA "); // print text 
    Serial.println("Pré rampa: " + String(active_stage));

  }else{
    lcd.print("RAMPA "); // print text 
    Serial.println("Rampa: " + String(active_stage));

  }
  lcd.print(active_stage); // print text 
}

 String returnActiveStage(int active_stage, boolean preRamp){
   String $msg;
  if(preRamp){
    $msg = "Pré rampa " + String(active_stage);

  }else{
    $msg = "Rampa " + String(active_stage);
  }
  return $msg;
 }

void showTime(unsigned long currentMillis, unsigned long brew_init_time){

  lcd.setCursor (0,2); //character 0, line 3
  lcd.print("Tempo: "); // print text 

  //Conversão do tempo para ser exibido
  unsigned long seconds = (currentMillis - brew_init_time) / 1000;
  unsigned long minutes = seconds / 60;
  seconds %= 60;
  lcd.setCursor (9,2); //character 0, line 3
  lcd.print(minutes);
  lcd.print(':');
  if (seconds < 10) {
    lcd.print('0');
  }
  lcd.print(seconds);
}

// Check if temperature is below threshold and if it needs to trigger output
void turnOnResistence(float temperature, int output){
  if(temperature != -127.00){
    String message = String("Temperature below threshold(") + String(temperature_array[active_stage].toFloat()) + ("). Current temperature: ") + 
    String(temperature) + String("C");
    Serial.println(message);
    //Turn on resistence
    Serial.println("Resistência ativada");
    digitalWrite(output, HIGH);
  }
}

// Check if temperature is above threshold and if it needs to trigger output
void turnOffResistence(float temperature, int output){
  String message = String("Temperature above threshold(") + String(temperature_array[active_stage].toFloat()) + ("). Current temperature: ") + 
            String(temperature) + String("C");
  Serial.println(message);
  //Turn off resistence
  Serial.println("Resistência desativada");
  digitalWrite(output, LOW);
}

void initRamp(float temperature){
  //Inicia contagem de tempo da rampa
  init_clock = 0;
  //Desativa pré rampa
  preRamp = 0;
  Serial.println("Iniciando rampa: " + String(active_stage));
  //Se a temperatura ambiente for maior ou igual a temperatura alvo e se a rampa não foi iniciada, a rampa inicia
  while (init_clock == 0)
  {
      ramp_start_time = millis();
      Serial.println("ramp_start_time: " + String(ramp_start_time));
      init_clock = 1;
  }
}

void finishBrew(int output){
    brew_started = 0;
    Serial.println("Brassagem finalizada");
    Serial.println("Resistência desativada");
    lcd.setCursor (0,4); //character 4, line 2
    lcd.print("Brassagem finalizada"); // print text  


    digitalWrite(output, LOW);
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
              recipeName = _activeStatus["nome"].as<String>();

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

static const char ACTIVE_STATUS[] PROGMEM = "{\"nome\":{{nome}},\"quantidade_rampas\":{{quantidade_rampas}},\"tempo_1\":{{tempo_1}},\"temperatura_1\":\"{{temperatura_1}}\" }";

String getJson()
{
    String active_status = FPSTR(ACTIVE_STATUS);
    active_status.replace("{{nome}}", String(recipeName));
    active_status.replace("{{quantidade_rampas}}", String(Quantidade_rampas));
    active_status.replace("{{tempo_1}}", String(time_array[0]));
    active_status.replace("{{temperatura_1}}", String(temperature_array[0]));

    return active_status;
}

String getRecipe(){
    String htmlRecipe;
    htmlRecipe =  "<h1>" + recipeName + "</h1>";
    htmlRecipe += "<h3>" + returnActiveStage(active_stage, preRamp) + "</h3>";
    htmlRecipe += "<ul style=\"list-style: none; text-align:left;\" > <li> Quantidade de rampas: " + Quantidade_rampas + "</li>";
    htmlRecipe += "<li> Tempo rampa 1: " + time_array[0] + " min</li>";
    htmlRecipe += "<li> Temperatura rampa 1: " + temperature_array[0] + " ºC</li></ul>";

    return htmlRecipe;
}
// GPIO where the output is connected to
const int output = 14;

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

  // Replaces placeholder with variable values
  String processor(const String& var){
    //Serial.println(var);
    if(var == "TEMPERATUREC"){
      return readDSTemperatureC();
    }
    else if(var == "RECEITA"){
      return getRecipe();
    }
    return String();
  }
void setup(){

  showWelcome();

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
    request->send(SPIFFS, "/index.html",String(), false, processor);
  });

    // Send a GET request to <IP>/get?message=<message>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String message;
      if (request->hasParam("brew_started")) {
          brew_started = request->getParam("brew_started")->value();
          brew_init_time = millis();
          message = "Brew started";
          // message =  getJson();
      } else {
          message = "Brew not started";
      }
      request->redirect("/");
      // request->send(200, "text/html", "Brew Started: " + message + "<br><a href=\"/\">Return to Home Page</a>");
  });

  server.on("/css/app.bb028a70.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/css/app.bb028a70.css", "text/css");
  });

  server.on("/css/chunk-vendors.367538c2.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/css/chunk-vendors.367538c2.css", "text/css");
  });

  server.on("/js/app.5509a63a.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/js/app.5509a63a.js");
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

  server.on("/receita", HTTP_GET, [](AsyncWebServerRequest *request)
  {
      // request->send(200, "application/json", getJson());
      request->send(200, "text/plain", getRecipe());
  });

}

void loop(){
  // Starts if
  if(brew_started){
    while(display_brew_msg)
    {
        Serial.println("Brew started");
        lcd.clear();
        lcd.print("Brew started");
        display_brew_msg = 0;
    }
    unsigned long currentMillis = millis();
    //Mensure temperature each 5 seconds
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
     
      // Temperature in Celsius degrees 
      sensors.requestTemperatures();
      float temperature = sensors.getTempCByIndex(0);

      showTemperature(temperature);
      showTime(currentMillis, brew_init_time); 
      
      //Se estiver em pré-rampa, acionar a resistência até chegar ao threshold
      if(preRamp){
        showActiveStage(active_stage, preRamp);
        digitalWrite(output, HIGH);
        if(temperature >= (temperature_array[active_stage].toFloat())){
          initRamp(temperature);
        }
      }else{
        showActiveStage(active_stage, preRamp);
        // if Horário atual >= Horário do fim da rampa
        if(currentMillis >= ((time_array[active_stage].toFloat() * 1000) + ramp_start_time)){
          active_stage ++;
          preRamp = 1;
        }else if(temperature >= (temperature_array[active_stage].toFloat() + 1)){
          turnOffResistence(temperature, output);
        }else if(temperature < (temperature_array[active_stage].toFloat() - 1)){
          turnOnResistence(temperature, output);
        }
        if(active_stage == (Quantidade_rampas.toInt())){
          finishBrew(output);
        }
      }
    }
  }
}
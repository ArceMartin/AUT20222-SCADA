// Automatización y Sistemas Embebidos 2022 - 2
// Proyecto Final
// 1103883 Martín Arce
// Jose Cimentales

// -------------------- DIRECTIVAS DE PREPROCESADOR FREERTOS -------------------

#if CONFIG_FREERTOS_UNICORE
  #define ARDUINO_RUNNING_CORE 0
#else
  #define ARDUINO_RUNNING_CORE 1
#endif

#ifndef LED_BUILTIN
  #define LED_BUILTIN 13
#endif

//------------------ DIRECTIVAS DE PREPROCESADOR DE WEB SERVER------------------

#include <ArduinoJson.h> // Serialización de objetos JSON
#include <driver/adc.h>  // Define enums para configurar ADC
#include <ESPmDNS.h>     // Protocolo multicast DNS para ESP32
#include <SPIFFS.h>      // SPI Flash File System. Sistema de archivos
#include <WebServer.h>   // Servidor Web. Soporta un solo cliente, GET/POST
#include <WiFi.h>        // Habilita comunicación WiFi
#include <WiFiClient.h>  // 
#include <Wire.h>        // Permite la comunicación I2C/TWI
#define DEVICE_NAME "ESP-32"
#define FIRMWARE_VERSION "1.0"

// -----------------------------------------------------------------------------

//Configuración de ESP en modo AP
const char *ssid        = "NETGEAR"; //"IZZI-145F";
const char *password    = ""; //"2WC456403972";
const char *ssid_AP     = "Esp32_AP";
const char *password_AP = "";
const char *htmlfile    = "/index.html";

float x;
float y;
float z;

WebServer server(80);
// AsyncWebServer server(80);


// -----------------------------------------------------------------------------

bool loadFromSpiffs(String path);
void handleRoot();
void handleWebRequests();
void getInfo();
void getInfo2();
void sensors();

// Tareas de FreeRTOS
void sensa(void *pvParameters );
void webserver(void *pvParameters );

// -----------------------------------------------------------------------------


void setup() {
  // ---------------------------- Setup de webserver ---------------------------

    // Configuración de ADC1 canal 6 y 7
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);

    vTaskDelay(pdMS_TO_TICKS(1000));
    Serial.begin(115200);
    Serial.println();

    // Inicializar el sistema de archivos
    SPIFFS.begin();
    Serial.println("Sistema de archivos inicializado");

    // Conexión con router WiFi
    WiFi.mode(WIFI_AP);  // WIFI_AP  WIFI_STA  WIFI_AP_STA
    WiFi.softAP(ssid_AP, password_AP);
    WiFi.begin(ssid, password);  

    Serial.println("");
    for (int k = 0; k < 20; k++) {
      if (WiFi.status() == WL_CONNECTED)
        break;
      // yield();
      vTaskDelay(pdMS_TO_TICKS(500));
      // yield();
      Serial.print(".");

    }
    yield();

    // Muestra datos de conexión en Serial Monitor
    Serial.println("");
    Serial.print("Conectado a ");
    Serial.println(ssid);
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());  // Dirección IP asignada a su ESP
    Serial.print("Dirección IP (Access Point): ");
    Serial.println(WiFi.softAPIP());

    // Inicializa respondedor mDNS para ESP-32-SCADA.local
    if (!MDNS.begin("ESP-32-SCADA")) {
      // En caso de error imprime mensaje y se queda ciclado
      Serial.println("Error al configurar el respondedor MDNS!");
      while (true)
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Inicializar el servidor web
    // Define las rutas y métodos a ejecutar
    server.on("/",HTTP_GET, handleRoot);
    server.on("/getInfo", HTTP_GET, getInfo);
    server.on("/sensors", HTTP_GET, sensors);
    // Si no encuentra la ruta, lo maneja como un URI (identificador de recurso/archivo)
    server.onNotFound(handleWebRequests); 
    server.begin();

    Serial.println("mDNS responder started");
    MDNS.addService("http", "tcp", 80);
    Wire.begin();

    Serial.println("Todo al 100%");

  // -------------------------- Inicia tasks de FreeRTOS -----------------------
  
  xTaskCreatePinnedToCore(sensa, "Sensores", 1024, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(webserver, "Servidor", 4096, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
}

// Loop vacío, se ejecutan tareas de FreeRTOS
void loop() {}

// -----------------------------------------------------------------------------

bool loadFromSpiffs(String path) {
  String dataType = "text/plain";
  if (path.endsWith("/")) path += "index.html";

  if (path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if (path.endsWith(".html")) dataType = "text/html";
  else if (path.endsWith(".htm")) dataType = "text/html";
  else if (path.endsWith(".css")) dataType = "text/css";
  else if (path.endsWith(".js")) dataType = "application/javascript";
  else if (path.endsWith(".png")) dataType = "image/png";
  else if (path.endsWith(".gif")) dataType = "image/gif";
  else if (path.endsWith(".jpg")) dataType = "image/jpeg";
  else if (path.endsWith(".ico")) dataType = "image/x-icon";
  else if (path.endsWith(".xml")) dataType = "text/xml";
  else if (path.endsWith(".pdf")) dataType = "application/pdf";
  else if (path.endsWith(".zip")) dataType = "application/zip";
  File dataFile = SPIFFS.open(path.c_str(), "r");
  if (server.hasArg("download")) dataType = "application/octet-stream";
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
  }

  dataFile.close();
  return true;
}

void handleRoot() {
  server.sendHeader("Location", "/index.html", true);  //Redirect to our html web page
  server.send(302, "text/plane", "");
}

void handleWebRequests() {
  if (loadFromSpiffs(server.uri())) return;
  String message = "File Not Detected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " NAME:" + server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.println(message);
}

void getInfo() {
  String json;
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["nombre"] = DEVICE_NAME;
  byte mac[6];  WiFi.macAddress(mac);
  root["mac"] = 
    String(mac[0], HEX)
    + ":" + String(mac[1], HEX)
    + ":" + String(mac[2], HEX)
    + ":" + String(mac[3], HEX)
    + ":" + String(mac[4], HEX)
    + ":" + String(mac[5], HEX);
  //root["fv"] = FIRMWARE_VERSION;
  root["fv"] = analogRead(A0);
  root["ssid"] = WiFi.SSID();
  IPAddress ip = WiFi.localIP();
  root["ip"] = String() + ip[0] + "." + ip[1] + "." + ip[2] + "." + ip[3];
  // root["masc"] = String() + 
  root.printTo(json);
  server.send(200, "text/json", json);
  Serial.println(json);
}

void getInfo2() {

  String json;
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["nombre"] = DEVICE_NAME;
  byte mac[6];  WiFi.macAddress(mac);
  root["mac"] = String(mac[5], HEX) + ":" + String(mac[4], HEX) + ":" + String(mac[3], HEX) + ":" + String(mac[2], HEX) + ":" + String(mac[1], HEX) + ":" + String(mac[0], HEX);
  //root["fv"] = FIRMWARE_VERSION;
  root["fv"] = String(millis());
  root["ssid"] = WiFi.SSID();
  IPAddress ip = WiFi.localIP();
  root["ip"] = String() + ip[0] + "." + ip[1] + "." + ip[2] + "." + ip[3];
  root.printTo(json);
  server.send(200, "text/json", json);
  Serial.println("se envia evento:" + json);
}

void sensors() {

  String json;
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["x"] = x;
  root["y"] = y;
  root["z"] = z;
  root.printTo(json);
  server.send(200, "text/json", json);
  Serial.println("se info 3:" + json);
}

void sensa(void *pvParameters) {
  (void) pvParameters;
  while(true) {
    // x = analogRead(4);
    x = adc1_get_raw((adc1_channel_t)6); // pin 34 adc1_6
    vTaskDelay(100);
    // x = map(x, 0.0, 4095.0, 0.0, 1.5);
    // vTaskDelay(100);

    // y = analogRead(0);
    y = adc1_get_raw((adc1_channel_t)7); // pin 35 adc1_7
    vTaskDelay(100);
    // y = map(y, 0.0, 4095.0, 0.0, 1.5);
    // vTaskDelay(100);

    z = analogRead(0);
    vTaskDelay(100);

    Serial.print(x);
    Serial.print("  ");
    Serial.print(y);
    Serial.print("  ");
    Serial.println(z);
    vTaskDelay(100);
  }
}

void webserver(void *pvParameters) {
  (void) pvParameters;

  while(true) {
    server.handleClient();
    vTaskDelay(100);
  }
}

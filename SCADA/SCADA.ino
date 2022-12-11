// Automatización / Sistemas Embebidos 2022 - 2
// Proyecto final
// Martín Arce 1103883
// Jose Cimentales 01163406

//------------------ DIRECTIVAS DE PREPROCESADOR PARA FREERTOS------------------

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

// --------------- DIRECTIVAS DE PREPROCESADOR PARA SERVIDOR WEB ---------------

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>
#define  DEVICE_NAME "ESP-32"
#define  FIRMWARE_VERSION "1.0"

// -----------------------------------------------------------------------------

void sensa(void *pvParameters);
void webserver(void *pvParameters);

// Función setup se ejecuta una vez al pulsar reset o encender la placa
void setup() {
  Serial.begin(115200);
  
  // Configuración de tareas ejecutadas en paralelo.

  xTaskCreatePinnedToCore(
    sensa
    ,  "Sensa Datos"
    ,  1024
    ,  NULL
    ,  1
    ,  NULL
    ,  ARDUINO_RUNNING_CORE
  );

  xTaskCreatePinnedToCore(
    webserver
    ,  "Servidor Web"
    ,  1024
    ,  NULL
    ,  2
    ,  NULL
    ,  ARDUINO_RUNNING_CORE
  );

  // Inicia automáticamente el programador de tareas (task scheduler)
  // asume el control de la programación de tareas individuales.
}

// loop vacío. 
void loop() // Vacío. Las cosas se hacen en tareas.
{
}

/*--------------------------------------------------*/
/*--------------------- Tareas ---------------------*/
/*--------------------------------------------------*/

void sensa(void *pvParameters) {
  while(true) {

  }
}

void webserver(void *pvParameters) {
  while(true) {

  }
}

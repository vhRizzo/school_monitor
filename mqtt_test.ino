#define DEBUG

#define tempo_bme 10
#define PressaoaNivelDoMar_HPA (1013.25)

#define WIFI_SSID "Herbert Richers"
#define WIFI_PASS "kinodertoten"

#define mqtt_server "test.mosquitto.org"
#define mqtt_port 1883
#define mqtt_user "admin123"
#define mqtt_pass "admin123"

#define temp_topic "bme280/temperatura"
#define umid_topic "bme280/umidade"
#define pres_topic "bme280/pressao"
#define alti_topic "bme280/altitude"

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_CCS811.h>

TaskHandle_t ccs_handle = NULL;

Adafruit_BME280 bme;
Adafruit_CCS811 ccs;

WiFiClient wifiClient;
PubSubClient MQTTclient(  mqtt_server, mqtt_port, wifiClient );

SemaphoreHandle_t sema_MQTT_KeepAlive;

byte mac[6];

void bme_task (void * param);
void ccs_task (void * param);
void mqtt_keepalive_task (void * param);

float global_umid;
float global_temp;

void setup() {
  Serial.begin(9600);

  sema_MQTT_KeepAlive = xSemaphoreCreateBinary();
  xSemaphoreGive( sema_MQTT_KeepAlive );
  
  bme.begin(0x76);
  ccs.begin(0x5A);
  
  global_umid = bme.readHumidity();
  global_temp = bme.readTemperature();
  
  xTaskCreate(mqtt_keepalive_task, "MQTT Keep Alive", 10000, NULL, 4, NULL);
  xTaskCreate(bme_task, "BME280", 10000, NULL, 3, NULL);
  xTaskCreate(ccs_task, "CCS811", 1500, NULL, 2, &ccs_handle);
} // setup()

void loop() {}

void bme_task (void * param) {
  float umid = 0.0f;
  float temp = 0.0f;
  float pres = 0.0f;
  float alti = 0.0f;

  while ( !MQTTclient.connected() ) vTaskDelay( 250 );
  
  for (;;) {
    umid = bme.readHumidity();
    temp = bme.readTemperature();
    pres = bme.readPressure() / 100.0f;
    alti = bme.readAltitude( PressaoaNivelDoMar_HPA );
    
    xSemaphoreTake( sema_MQTT_KeepAlive, portMAX_DELAY );
    if ( MQTTclient.connected() ) {
      MQTTclient.publish( temp_topic, String(temp).c_str() );
      vTaskDelay( 2 ); // gives the Raspberry Pi 4 time to receive the message and process
      MQTTclient.publish( umid_topic, String(umid).c_str() );
      vTaskDelay( 2 ); // no delay and RPi is still processing previous message
      MQTTclient.publish( pres_topic, String(pres).c_str() );
      vTaskDelay( 2 ); // no delay and RPi is still processing previous message
      MQTTclient.publish( alti_topic, String(alti).c_str() );
    }
    
    xSemaphoreGive( sema_MQTT_KeepAlive );
    xTaskNotifyGive( ccs_handle );
    
    #ifdef DEBUG
    Serial.print( "Temperatura: " );
    Serial.print( temp, 2 );
    Serial.print( ", Umidade: " );
    Serial.print( umid, 2 );
    Serial.print( ", Pressao: " );
    Serial.print( pres, 2 );
    Serial.print( ", Altitude: " );
    Serial.println( temp, 2 );
    #endif
    
    vTaskDelay( (tempo_bme*1e3f) / portTICK_PERIOD_MS );
  }
} // bme_task()

void ccs_task (void * param) {
  float eco2 = 0.0f;
  float tvoc = 0.0f;
  while (true) {
    ulTaskNotifyTake( pdTRUE, (1000 / portTICK_PERIOD_MS) ); // funciona com um semáforo
    ccs.setEnvironmentalData(global_umid, global_temp);
    ccs.readData();
    eco2 = ccs.geteCO2();
    tvoc = ccs.getTVOC();
    
    #ifdef DEBUG
    Serial.print( "CO2: " );
    Serial.print( eco2, 2 );
    Serial.print( ", TVOC: " );
    Serial.println( tvoc, 2 );
    #endif
    
    vTaskDelay( (tempo_bme*1e3f) / portTICK_PERIOD_MS );
  }
} // ccs_task()

void mqtt_keepalive_task ( void *pvParameters ) {
  // setting must be set before a mqtt connection is made
  MQTTclient.setKeepAlive( 90 ); // setting keep alive to 90 seconds makes for a very reliable connection, must be set before the 1st connection is made.
  for (;;) {
    //check for a is-connected and if the WiFi 'thinks' its connected, found checking on both is more realible than just a single check
    if ( (wifiClient.connected()) && (WiFi.status() == WL_CONNECTED) ) {
      xSemaphoreTake( sema_MQTT_KeepAlive, portMAX_DELAY ); // whiles MQTTlient.loop() is running no other mqtt operations should be in process
      MQTTclient.loop();
      xSemaphoreGive( sema_MQTT_KeepAlive );
    }
    else {
      
      #ifdef DEBUG
      Serial.print( "MQTT keep alive found MQTT status " );
      Serial.print( String(wifiClient.connected()) );
      Serial.print( " WiFi status" );
      Serial.println( String(WiFi.status()) );
      #endif
      
      if ( !(WiFi.status() == WL_CONNECTED) ) connectToWiFi();
      connectToMQTT();
    }
    vTaskDelay( 250 ); //task runs approx every 250 mS
  }
  vTaskDelete ( NULL );
} // mqtt_keepalive_task()

void connectToMQTT() {
  // create client ID from mac address
  String clientID = String(mac[0]) + String(mac[5]) ;
  
  #ifdef DEBUG
  Serial.print( "connect to mqtt as client " );
  Serial.println( clientID );
  #endif
  
  while ( !MQTTclient.connected() ) {
    MQTTclient.connect( clientID.c_str(), mqtt_user, mqtt_pass );

    #ifdef DEBUG
    Serial.println ( "connecting to MQTT" );
    #endif
    
    vTaskDelay( 250 );
  }

  #ifdef DEBUG
  Serial.println( "MQTT Connected" );
  #endif
  
} // connectToMQTT

void connectToWiFi() {

  #ifdef DEBUG
  Serial.println( "connect to wifi" );
  #endif
  
  while ( WiFi.status() != WL_CONNECTED ) {
    WiFi.disconnect();
    WiFi.begin( WIFI_SSID, WIFI_PASS );

    #ifdef DEBUG
    Serial.println(" waiting on wifi connection" );
    #endif
    
    vTaskDelay( 4000 );
  }

  #ifdef DEBUG
  Serial.println( "Connected to WiFi" );
  #endif
  
  WiFi.macAddress(mac);

  #ifdef DEBUG
  Serial.print( "mac address " );
  Serial.print( mac[0] );
  Serial.print( "." );
  Serial.print( mac[1] );
  Serial.print( "." );
  Serial.print( mac[2] );
  Serial.print( "." );
  Serial.print( mac[3] );
  Serial.print( "." );
  Serial.print( mac[4] );
  Serial.print( "." );
  Serial.println( mac[5] );
  #endif
  
  WiFi.onEvent( WiFiEvent );
} // connectToWiFi

void WiFiEvent( WiFiEvent_t event ){
  switch ( event ) {
    case SYSTEM_EVENT_STA_CONNECTED:
      #ifdef DEBUG
      Serial.println("Connected to access point");
      #endif
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      #ifdef DEBUG
      Serial.println("Disconnected from WiFi access point");
      #endif
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      #ifdef DEBUG
      Serial.println("WiFi client disconnected");
      #endif
      break;
    default: break;
  }
} // WiFiEvent()

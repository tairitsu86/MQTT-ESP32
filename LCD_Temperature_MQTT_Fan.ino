#include "DHT.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <WiFi.h>
extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
}
#include <AsyncMqttClient.h>

#define DHTPIN 26
#define DHTTYPE DHT22   // DHT 22 如果用的是DHT22，就用這行
DHT dht(DHTPIN, DHTTYPE);


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

//------------------------------
//replace with your network credentials
#define WIFI_SSID "ILKiyohime"
#define WIFI_PASSWORD "ILKiyohime"

// Raspberry Pi Mosquitto MQTT Broker
#define MQTT_HOST IPAddress(140, 136, 151, 74)
#define MQTT_PORT 1883

//MQTT Topics
#define MQTT_Control "esp32/Control"

#define POWER 17
int timeCounter = 0;
boolean autoMode = 1;
int tempLimit = 28;

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
    
void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %dn", event);
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
  }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  
  uint16_t packetIdSub = mqttClient.subscribe(MQTT_Control, 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSub);
}


void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}
void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}


void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("\n Publish received.");
  Serial.print("topic: ");
  Serial.println(topic);
  String messageTemp;
  for (int i = 0; i < len; i++) {
    messageTemp += (char)payload[i];
  }
    Serial.print("Message: ");
    Serial.println(messageTemp);

  if (messageTemp == "ON"){
  autoMode=0;
  digitalWrite(POWER, HIGH); 
  Serial.println("Power is now ON!");
  publishData("esp32/State","ON");
  }else if(messageTemp == "OFF"){
  autoMode=0;
  digitalWrite(POWER, LOW); 
  Serial.println("Power is now OFF");
  publishData("esp32/State","OFF");
  }else if(messageTemp == "AUTO"){
  autoMode=1;
  Serial.println("Power is AUTO");
  publishData("esp32/State","AUTO");
  }
}

void publishData(char* topic,char* data){
  Serial.println("Publishing data");
  uint16_t packetId = mqttClient.publish(topic, 1, true, data);
  Serial.print("Publish done, packetId: ");
  Serial.println(packetId);
}
void setup() {
  Serial.begin(115200);
  pinMode(POWER, OUTPUT);

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  connectToWifi();

  dht.begin();
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  timeCounter=0;
}
char tmp[8];
void loop(){
  
  float h = dht.readHumidity();   //取得濕度
  float t = dht.readTemperature();  //取得溫度C

  //顯示在監控視窗裡
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C ");

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE); 
  display.setCursor(0,0);
  display.print(F("H: "));
  display.print(h);
  display.println(F("%"));
  display.print(F("T: "));
  display.print(t);
  display.print(F("C"));
  display.display();
  if(autoMode){
    if(t>=tempLimit){
        digitalWrite(POWER, HIGH); 
  Serial.println("Power is now ON!");
  publishData("esp32/State","ON");  
    }else{

  digitalWrite(POWER, LOW); 
  Serial.println("Power is now OFF");
  publishData("esp32/State","OFF");
    }
  }

  if(timeCounter%10==0){
    dtostrf(h, 2, 2, tmp);
    publishData("esp32/Humidity",tmp);
    dtostrf(t, 2, 2, tmp);
    publishData("esp32/Temperature",tmp);
  }
  delay(1000);
  if(timeCounter>=86400)
    timeCounter=0;
  else
    timeCounter++;
}
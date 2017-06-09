/*
 * Description:  control relay with ssdp and mqtt 
 * 
 * Author: ninja
 * 
 * Date: 2017-05-06
 * 
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "SSDPClient.h"
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#include <WiFiManager.h>
#include <DNSServer.h>


//const char* ssid = "brofi_office";
//const char* password = "Mmjsjg9+@";
const char* ssid = "CU_3HVb";
const char* password = "dygu3kgf";
//const char* mqtt_server = "192.168.3.133";

WiFiClient espClient;
PubSubClient MQTTClient(espClient);

//#define DEBUG_WiFi  Serial
//#define DEBUG_MQTT  Serial
//#define DEBUG_JSON  Serial
#define DEBUG_WM_ON   true
#define DEBUG_WM_OFF  false

#define SSDP_PORT 1883

const char* subTopic = "inTopic";
const char* pubTopic = "outTopic";

const char* device_name = "switch1";

#define SW  4 //nodemcu D2

#define LED_SSDP  14  //nodemcu D5
#define LED_MQTT  12  //nodemcu D6
#define LED_RELAY 13  //nodemcu D7

#define LED_MQTT_ON   digitalWrite(LED_MQTT, LOW)
#define LED_MQTT_OFF  digitalWrite(LED_MQTT, HIGH)

#define LED_RELAY_ON  digitalWrite(LED_RELAY, LOW)
#define LED_RELAY_OFF digitalWrite(LED_RELAY, HIGH)

//volatile bool MQTT_Status = false;

#if 0
long lastMsg = 0;
char msg[50];
int value = 0;

String gssdp_notify_template =
  "NOTIFY * HTTP/1.1\r\n"
  "Host: 239.255.255.250:1900\r\n"
  "Cache-Control: max-age=2\r\n"
  "Location: 192.168.1.35\r\n"
  "Server: Linux/#970 SMP Mon Feb 20 19:18:29 GMT 2017 GSSDP/0.14.10\r\n"
  "NTS: ssdp:alive\r\n"
  "NT: upnp:rootdevice\r\n"
  "USN: uuid:5911c26e-ccc3c-5421-3721-b827eb3ea653::urn:schemas-upnp-org:service:voice-master:1\r\n";

#endif

void portInit(){
  pinMode(SW, OUTPUT);
  digitalWrite(SW, LOW);

  pinMode(LED_SSDP, OUTPUT);
  digitalWrite(LED_SSDP, HIGH);

  pinMode(LED_MQTT, OUTPUT);
  digitalWrite(LED_MQTT, HIGH);

  pinMode(LED_RELAY, OUTPUT);
  digitalWrite(LED_RELAY, HIGH);

}

void TurnON(){
  digitalWrite(SW, HIGH);
  LED_RELAY_ON;
}

void TurnOFF(){
  digitalWrite(SW, LOW);
  LED_RELAY_OFF;
}


void Scan_WiFi(){

   // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
    }
  }
  Serial.println("");

  // Wait a bit before scanning again
  delay(5000);
}

void SmartConnect(){
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(DEBUG_WM_OFF);
  wifiManager.autoConnect("MonKing","MonKing123");

#ifdef DEBUG_WiFi
  DEBUG_WiFi.println("smart connect... : )");
#endif
}

void Connect_WiFi(){

  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

#ifdef DEBUG_WiFi
  DEBUG_WiFi.println("");
  DEBUG_WiFi.println("IP address: ");
  DEBUG_WiFi.println(WiFi.localIP());
#endif
}

bool jsonPaser(byte* payload){
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.parse(payload);
  if(!data.success()){
    return false;
  }
  const char* name = data["name"];
  const char* target_id = data["target_id"];
  const char* action = data["action"];
  const char* value = data["value"];

#ifdef DEBUG_JSON
    DEBUG_JSON.println(name);
    DEBUG_JSON.println(target_id);
    DEBUG_JSON.println(action);
    DEBUG_JSON.println(value);
#endif

  if(!strcmp(value,"0")){
    Serial.println("turn off...");
    TurnOFF();
  }else if(!strcmp(value,"1")){
    Serial.println("turn on...");
    TurnON();
  }else{
    return false;
  }

  return true;
  
}

void callback(char* topic, byte* payload, unsigned int length){

#ifdef DEBUG_MQTT
  DEBUG_MQTT.print("Message arrived [");
  DEBUG_MQTT.print(topic);
  DEBUG_MQTT.print("] ");
 
  for (int i = 0; i < length; i++) {
    DEBUG_MQTT.print((char)payload[i]);
  }
  DEBUG_MQTT.println();
 #endif 
 
  jsonPaser(payload);

}

void reconnect() {
  // Loop until we're reconnected
  while (!MQTTClient.connected()) {
 #ifdef DEBUG_MQTT
    DEBUG_MQTT.println("Attempting MQTT connection...");
 #endif
    MQTTSetup();
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (MQTTClient.connect(clientId.c_str())) {
#ifdef DEBUG_MQTT
      DEBUG_MQTT.println("mqtt broker connected");
#endif
      // Once connected, publish an announcement...
      MQTTClient.publish(pubTopic, "hello world");
      // ... and resubscribe
      MQTTClient.subscribe(subTopic);
    } else {

#ifdef DEBUG_MQTT
      DEBUG_MQTT.print("failed, rc=");
      DEBUG_MQTT.print(MQTTClient.state());
      DEBUG_MQTT.println(" try again in 5 seconds");
#endif
      delay(5000);
    }
  }
}

bool MQTTSetup(){
  Serial.println("start MQTTSetup...");
  char mqtt_server[255] = "";
  String location = SSDPClient.getLocation();
  if(location == ""){
    return false;
  }
  location.toCharArray(mqtt_server,location.length()+1);

 #ifdef DEBUG_MQTT
  DEBUG_MQTT.println(mqtt_server);
#endif
  MQTTClient.setServer(mqtt_server, SSDP_PORT);
  MQTTClient.setCallback(callback);
  return true;
}

void SSDPClientSetup(){
    SSDPClient.schema(espClient);
    SSDPClient.begin();
}


void setup(){
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  portInit();
  
//  WiFi.mode(WIFI_STA);
//  WiFi.disconnect();
//  delay(100);
//  Connect_WiFi();
  SmartConnect();
  SSDPClientSetup();
  MQTTSetup();
  Serial.println("Setup Done ...");
}

void loop(){
  if(!MQTTClient.connected()){
    LED_MQTT_OFF;
    reconnect();
  }else{
    LED_MQTT_ON;
  }

  MQTTClient.loop();

//  long now = millis(); 
//  if (now - lastMsg > 2000) {
//    lastMsg = now;
//    ++value;
//    snprintf (msg, 75, "hello world #%ld", value);
//    Serial.print("Publish message: ");
//    Serial.println(msg);
//    MQTTClient.publish("outTopic", msg);
//  }
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}


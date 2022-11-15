// RELAY
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ConfigPortal8266.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>

char*               ssid_pfix = (char*)"jongminPortal";
String              user_config_html = ""
                "<p><input type='text' name='yourVar placeholder='Your Variable'"; 

#define RELAY 15

const char*   ssid="533-2.4G-4";
const char*   password = "kpu123456!";

ESP8266WebServer server(80);

const char*   mqttServer = "iotlab101.tosshub.co";
const int     mqttPort = 1883;

unsigned long interval = 3000;
unsigned long lastPublished = - interval;

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length);
void pubStatus();

void handleRoot(){
  String message = (server.method() == HTTP_GET)?"GET":"POST";
  message += " " + server.uri() + "\n";
  for(uint8_t i=0; i < server.args(); i++){
    message += " " + server.argName(i) + " : " + server.arg(i) + "\n";
  }
  message += "\nHello from ESP8266\n";
  server.send(200, "text/plain", message);

}

void handleNotFound(){
  String message = "File Not Found\n\n";
  server.send(404, "text/plain", message);
}


void setup() {
  Serial.begin(115200);
  pinMode(RELAY, OUTPUT);
  loadConfig();
  if(!cfg.containsKey("config") || strcmp((const char*)cfg["config"], "done")) {
        configDevice();
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while( WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.printf("\nIP address : "); Serial.println(WiFi.localIP());

  if (MDNS.begin("jongminPortal")) {
      Serial.println("MDNS responder started");
  }    
  
  server.on("/", handleRoot);

  server.on("/inline", [](){
    server.send(200, "text/plain", "Hello from the inline function\n");
  });

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  Serial.println("Connected to the WiFi network");
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  while(!client.connected()){
    Serial.println("Connecting to MQTT...");

    if (client.connect("jongminKim_RELAY")){
      Serial.println("connected");
    } else {
      Serial.print("failed with state"); Serial.println(client.state());
      delay(2000);
    }
  }
  client.subscribe("id/jongminKim/relay/cmd");
  client.subscribe("id/jongminKim/sensor/cmd/humidity");
  // client.subscribe("id/jongminKim/sensor/cmd/temperature");
  // client.subscribe("id/jongminKim/sensor/cmd/light");
  digitalWrite(RELAY, LOW);
}

void loop() {
  MDNS.update();
  server.handleClient();
  client.loop();
  
  unsigned long currentMillis = millis();
  if(currentMillis - lastPublished >= interval){
    lastPublished = currentMillis;
    pubStatus();
  }
}

void pubStatus() {
  char buf[10];
  if (digitalRead(RELAY) == HIGH){
    sprintf(buf, "on");
  } else {
    sprintf(buf, "off");
  }

  // client.publish("id/jongminKim/relay/evt", buf);
}

void callback(char* topic, byte* payload, unsigned int length){
  char msgBuffer[20];
  if(!strcmp(topic, "id/jongminKim/relay/cmd")){
    int i;
    for(i = 0; i < (int)length; i++){
      msgBuffer[i] = payload[i];
    }

    msgBuffer[i] = '\0';
    Serial.printf("\n%s -> %s", topic, msgBuffer);
    if(!strcmp(msgBuffer, "on")){
      digitalWrite(RELAY, HIGH);
    } else if(!strcmp(msgBuffer, "off")){
      digitalWrite(RELAY, LOW);
    }
  }
  pubStatus();
}
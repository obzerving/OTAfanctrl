/*
 * OTAfanctrl - Control a fan remote
 * Receives commands in the form
 * http://ESP_000000/zwh?params=off
 * or
 * http://ESP_000000/zwh?params=on,${intensity.percent}
 * Sends status in the form
 * http://zwhomeserver:8000/devices/<device name>?status=[off,on]&level=${intensity.percent}
 */
#include <ESP8266WiFi.h>
#include <aREST.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>
#define clickspeed 100
struct ctrl_struct {
  char ctrl_name[10];
  byte ctrl_pin;
};
ctrl_struct fan[5];
aREST rest = aREST();
const char* ssid = "your_ssid";
const char* password = "your_password";
const char* zwhome = "http://zwhomeserver:8000/devices/";
const char* thisnode = "bedroom.fan";
#define LISTEN_PORT           80
WiFiServer server(LISTEN_PORT);

// Variables to be exposed to the API
int devstate; // 0=off, 1=low, 2=med, 3=high
int level; // A percentage from Amazon Echo command

// Declare functions to be exposed to the API
int pushbutton(String command);
int zwh(String command);

void setup(void)
{
  // Start Serial
  Serial.begin(115200);
  strcpy(fan[0].ctrl_name, "FAN-OFF");
  fan[0].ctrl_pin = 5;
  strcpy(fan[1].ctrl_name, "FAN-LOW");
  fan[1].ctrl_pin = 12;
  strcpy(fan[2].ctrl_name, "FAN-MED");
  fan[2].ctrl_pin = 13;
  strcpy(fan[3].ctrl_name, "FAN-HIGH");
  fan[3].ctrl_pin = 14;
  strcpy(fan[4].ctrl_name, "FAN-LIGHT");
  fan[4].ctrl_pin = 2;

  for(int i=0; i<5; i++) {
    pinMode(fan[i].ctrl_pin, OUTPUT);
    digitalWrite(fan[i].ctrl_pin, LOW);
  }
  EEPROM.begin(512);
  byte value = EEPROM.read(0); // Check magic word
  if(value != B01100110)
  { // Not there, so set up EEPROM
    EEPROM.write(0, B01100110); // Magic word
    EEPROM.write(1, 0); // Current value of devstate
    EEPROM.write(2, 1); // Last "on" setting of devstate (init to Low)
    EEPROM.commit();
    devstate = 0;
  }
  else {
    devstate = EEPROM.read(1); // restore device state
  }
  level = devstate * 25;
  rest.variable("state", &devstate);
  rest.variable("level", &level);
  // Functions to be exposed
  rest.function("zwh", zwh);
  // Give name & ID to the device (name < 20 chars and id < 7)
  rest.set_id("1");
  rest.set_name("bedroom.fan");
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  // Start the server
  server.begin();
  Serial.println("Server started");
  // Print the IP address
  Serial.println(WiFi.localIP());
  ArduinoOTA.onStart([]() {
    String type;
    Serial.println("\nStart");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();
  // Handle REST calls
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  while(!client.available()){
    delay(1);
  }
  rest.handle(client);
}

int zwh(String command) {
  // Get state and level from command (e.g. on,100)
  int i1 = command.indexOf(',');
  int pct;
  int ds;
  if(i1 == -1) { // no comma.
    if(command.substring(0) == "off") { // off command doesn't have a comma
      ds = 0;
      pct = 0;
    }
    if(command.substring(0) == "on") { // No level specified
       ds = EEPROM.read(2); // Use last devstate "on" setting
       pct = ds * 25; // and convert to a level within the range of devstate
    }
  } // i1 == -1
  else { // We have comma separated params (assume we have an on command)
    i1++; // point to beginning of level
    String lvl = command.substring(i1);
    if(lvl.length() > 0) {
      pct = lvl.toInt();
    }
    else { // No level param sent
     ds = EEPROM.read(2); // Use last devstate "on" setting
     pct = ds * 25; // and convert to a level within the range of devstate
    }
  }
  level = pct; // update level and devstate
  if(devstate > 0) EEPROM.write(2, devstate);
  if(pct == 0) devstate = 0;
  if((pct > 0) && (pct < 31)) devstate = 1;
  if((pct > 30) && (pct < 61)) devstate = 2;
  if(pct >= 60) devstate = 3;
  // Push the appropriate button
  digitalWrite(fan[devstate].ctrl_pin,HIGH);
  delay(clickspeed);
  digitalWrite(fan[devstate].ctrl_pin,LOW);
  delay(clickspeed);
  EEPROM.write(1, devstate);
  EEPROM.commit();
  char smsg[100];
  HTTPClient http;
  snprintf(smsg, sizeof(smsg)-1, "%s%s?status=%d&level=%d", zwhome, thisnode, devstate, level);
  http.begin(smsg);
  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();
  // httpCode will be negative on error
  if(httpCode > 0) {
    // HTTP header has been sent and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    // file found at server
    if(httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
//    Serial.println(payload);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  return devstate;
}

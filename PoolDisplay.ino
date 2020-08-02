
//initial status on startup is a bit of a hack
//we "turn on" a virtual switch in openhab via mqtt, which fires a rule that dumps the status of temperature, heat, pump, light
//to separeate topics which we read and update on the display.
//
//seems there should be a more proper way to do this with state topic vs command topic - to investigate

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "credentials.h"
#include "oled-display.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define HOSTNAME "PoolDisplay"

Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN);

char* overwatchTopic = "kolcun/outdoor/pool/display/overwatch";
char* controlTopic = "kolcun/outdoor/pool/display";
char* stateTopic = "kolcun/outdoor/pool/display/state";
char* pumpTopic = "kolcun/outdoor/pool/pump";
char* lightTopic = "kolcun/outdoor/pool/light";
char* heaterTopic = "kolcun/outdoor/pool/heater";
char* temperatureTopic = "kolcun/outdoor/pool/temperature/state";
char* requestStatusTopic = "kolcun/outdoor/pool/requeststatus";
char* heaterStatusTopic = "kolcun/outdoor/pool/status/heater";
char* lightStatusTopic = "kolcun/outdoor/pool/status/light";
char* pumpStatusTopic = "kolcun/outdoor/pool/status/pump";
char* server = MQTT_SERVER;
char* mqttMessage;
char* pumpSpeed = "N/A";
char* lightState = "N/A";
char* heaterState = "N/A";
float poolTemperature = 0;

WiFiClient wifiClient;
PubSubClient pubSubClient(wifiClient);

StaticJsonDocument<200> doc;

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  setupOTA();
  setupDisplay();

  pinMode(LED_BUILTIN, OUTPUT);

  pubSubClient.setServer(server, 1883);
  pubSubClient.setCallback(mqttCallback);

  if (!pubSubClient.connected()) {
    reconnect();
  }
  getInitialStatus();
  refreshDisplay();
}

void loop() {
  ArduinoOTA.handle();

  if (!pubSubClient.connected()) {
    reconnect();
  }
  pubSubClient.loop();

}

void refreshDisplay() {
  tft.fillScreen(BLACK);
  //tft.setFont(&FreeSans12pt7b);
  //tft.setFont(&FreeSans9pt7b);
  tft.setCursor(0, 0);
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.println("Pool");
  tft.print(poolTemperature, 1);
  tft.setTextSize(1);
  tft.print("C");
  tft.setTextSize(3);
  tft.println("");
  tft.setTextSize(2);
  tft.println("");
  tft.print("Pump: ");
  tft.println(pumpSpeed);

  tft.println("");
  tft.setTextColor(GREY);

  tft.setTextColor(BLACK);
  if (heaterState == "OFF") {
    tft.setTextColor(GREY);
  } else if (heaterState == "ON") {
    tft.setTextColor(RED);
  }
  tft.print("HEAT ");

  tft.setTextColor(BLACK);
  if (lightState == "OFF") {
    tft.setTextColor(GREY);
  } else if (lightState == "ON") {
    tft.setTextColor(YELLOW);
  }
  tft.println("LIGHT");

}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print(F("MQTT Message Arrived ["));
  Serial.print(topic);
  Serial.print(F("] "));
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.print("length: ");
  Serial.println(length);

  mqttMessage = (char*) payload;

  if (strcmp(topic, controlTopic) == 0) {
    if (strncmp(mqttMessage, "refresh", length)  == 0) {
      Serial.println("refresh display");
      refreshDisplay();
    }
    if (strncmp(mqttMessage, "invert-yes", length)  == 0) {
      tft.invert(true);
    } else if (strncmp(mqttMessage, "invert-no", length)  == 0) {
      tft.invert(false);
    }
  }

  if (strcmp(topic, pumpTopic) == 0 || strcmp(topic, pumpStatusTopic) == 0) {
    if (strncmp(mqttMessage, "Speed1", length) == 0 || strncmp(mqttMessage, "0", length) == 0) {
      pumpSpeed = "Off";
    } else if (strncmp(mqttMessage, "Speed2", length) == 0 || strncmp(mqttMessage, "25", length) == 0) {
      pumpSpeed = "Skim";
    } else if (strncmp(mqttMessage, "Speed3", length) == 0 || strncmp(mqttMessage, "50", length) == 0) {
      pumpSpeed = "Heat";
    } else if (strncmp(mqttMessage, "Speed4", length) == 0 || strncmp(mqttMessage, "100", length) == 0) {
      pumpSpeed = "Max";
    }
    refreshDisplay();
  }

  if (strcmp(topic, lightTopic) == 0 || strcmp(topic, lightStatusTopic) == 0) {
    if (strncmp(mqttMessage, "OFF", length) == 0) {
      lightState = "OFF";
    } else if (strncmp(mqttMessage, "ON", length) == 0) {
      lightState = "ON";
    }
    refreshDisplay();
  }

  if (strcmp(topic, heaterTopic) == 0 || strcmp(topic, heaterStatusTopic) == 0) {
    if (strncmp(mqttMessage, "OFF", length) == 0) {
      heaterState = "OFF";
    } else if (strncmp(mqttMessage, "ON", length) == 0) {
      heaterState = "ON";
    }
    refreshDisplay();
  }

  if (strcmp(topic, temperatureTopic) == 0) {
    payload[length] = '\0';
    deserializeJson(doc, payload);
    float oldTemp = poolTemperature;
    float f = doc["temperature"]["C"];
    Serial.println(f);
    Serial.print("Old Temp: ");
    Serial.println(oldTemp);
    Serial.print("New Temp: ");
    Serial.println(f);
    if (oldTemp != f) {
      Serial.println("temp change, update");
      poolTemperature = f;
      refreshDisplay();
    }else{
      Serial.println("no change, no update");
    }
  }

}

void getInitialStatus() {
  //pubSubClient.publish(requestStatusTopic, "1");
}

void reconnect() {
  while (!pubSubClient.connected()) {
    if (pubSubClient.connect("pool-display", MQTT_USER, MQTT_PASSWD)) {
      Serial.println("Connected to MQTT broker");
      pubSubClient.publish(overwatchTopic, "Pool Display online");
      Serial.print("sub to: '");
      Serial.print("kolcun/outdoor/pool/#");
      Serial.println("'");
      if (!pubSubClient.subscribe("kolcun/outdoor/pool/#", 1)) {
        Serial.println("MQTT: unable to subscribe to control topic");
      }

    } else {
      Serial.print("failed, rc=");
      Serial.print(pubSubClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

}

void setupDisplay() {
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(BLACK);
}

void setupOTA() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Wifi Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.setHostname(HOSTNAME);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

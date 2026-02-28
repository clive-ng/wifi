#include <WiFi.h>
#include <PubSubClient.h> // Install this library!

// --- WiFi Settings ---
const char* ssid = "";
const char* password = "";

// --- MQTT Settings ---
const char* mqtt_server = "broker.hivemq.com";
const char* topic = "exhibit/clive/lights"; // Change "clive" to something unique

const int ledPin = 2;
int currentMode = 0;
bool ledState = false;
unsigned long previousMillis = 0;

WiFiClient espClient;
PubSubClient client(espClient);

// This runs when a message arrives from the 4G network
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String message = String((char*)payload);
  currentMode = message.toInt();
  
  if (currentMode == 0) digitalWrite(ledPin, LOW);
  if (currentMode == 1) digitalWrite(ledPin, HIGH);
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32_Clive_Client")) {
      client.subscribe(topic);
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // Blink logic remains the same
  if (currentMode == 2 || currentMode == 3) {
    unsigned long currentMillis = millis();
    int speed = (currentMode == 2) ? 1000 : 100;
    if (currentMillis - previousMillis >= speed) {
      previousMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
    }
  }
}

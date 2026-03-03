#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// --- WiFi Credentials ---
const char* ssid = "Point";
const char* password = "ucaclive";

// --- Static IP Configuration (Matches your 192.168.0.1 Router) ---
IPAddress local_IP(192, 168, 0, 50);   // The IP for your ESP32
IPAddress gateway(192, 168, 0, 1);    // Your Router's IP from your photo
IPAddress subnet(255, 255, 255, 0);   
IPAddress primaryDNS(8, 8, 8, 8);     // Google DNS

// --- Hardware Settings ---
const int ledPin = 2; // Internal LED on most ESP32 boards
int currentMode = 0;  // 0=Off, 1=On, 2=Slow, 3=Fast
bool ledState = false;
unsigned long previousMillis = 0;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// --- WebSocket Logic ---
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;
    currentMode = message.toInt();
    
    Serial.print("New Mode Received: ");
    Serial.println(currentMode);

    // Immediate reaction for static modes
    if (currentMode == 0) digitalWrite(ledPin, LOW);
    if (currentMode == 1) digitalWrite(ledPin, HIGH);
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("Client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("Client disconnected");
  } else if (type == WS_EVT_DATA) {
    handleWebSocketMessage(arg, data, len);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Apply Static IP Settings
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("STA Failed to configure Static IP");
  }

  // Connect to your Home WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Connected!");
  Serial.print("Use this IP in your HTML: ");
  Serial.println(WiFi.localIP());

  // Initialize WebSockets
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Allow the external HTML (from GitHub or Local File) to talk to the ESP32
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  server.begin();
}

void loop() {
  ws.cleanupClients();

  // Non-blocking blink logic
  if (currentMode == 2 || currentMode == 3) {
    unsigned long currentMillis = millis();
    int blinkSpeed = (currentMode == 2) ? 1000 : 100;
    
    if (currentMillis - previousMillis >= blinkSpeed) {
      previousMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
    }
  }
}

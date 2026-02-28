#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// --- WiFi Settings ---
const char* ssid = "Exhibit_Light_Control";
const char* password = "password123";

// --- Hardware Settings ---
const int ledPin = 2; // Pin 2 is usually the built-in blue LED on the ESP32
int currentMode = 0;  // 0=Off, 1=On, 2=Slow Blink, 3=Fast Blink
bool ledState = false;
unsigned long previousMillis = 0; // Used for non-blocking timers

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// --- The HTML/CSS/JS Web App ---
// We put the Figma-style code directly here so you don't need SPIFFS for this test!
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Lighting Control</title>
  <style>
    body { font-family: sans-serif; background-color: #2c3e50; color: white; text-align: center; padding-top: 50px; }
    .card { background: #34495e; padding: 2rem; border-radius: 15px; display: inline-block; box-shadow: 0 10px 20px rgba(0,0,0,0.3); }
    button { border: none; padding: 15px 30px; margin: 10px; border-radius: 50px; font-size: 1.1rem; cursor: pointer; color: white; transition: 0.2s; }
    button:active { transform: scale(0.95); }
    .btn-off { background-color: #e74c3c; }
    .btn-on { background-color: #2ecc71; }
    .btn-blink { background-color: #f39c12; }
  </style>
</head>
<body>
  <div class="card">
    <h1>Exhibit Lighting</h1>
    <p>Tap a button to change the ESP32 sequence.</p>
    <button class="btn-off" onclick="sendSequence('0')">Turn OFF</button>
    <button class="btn-on" onclick="sendSequence('1')">Turn ON</button><br>
    <button class="btn-blink" onclick="sendSequence('2')">Slow Pulse</button>
    <button class="btn-blink" onclick="sendSequence('3')">Fast Strobe</button>
  </div>

  <script>
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;
    
    // Connect to the ESP32 WebSocket as soon as the page loads
    window.onload = function() {
      websocket = new WebSocket(gateway);
    };

    // Send the chosen mode to the ESP32
    function sendSequence(mode) {
      websocket.send(mode);
      console.log("Sent mode: " + mode);
    }
  </script>
</body>
</html>
)rawliteral";

// --- WebSocket Listener ---
// This triggers whenever the phone clicks a button
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0; // Null-terminate the string
    String message = (char*)data;
    
    currentMode = message.toInt(); // Convert the "0", "1", "2", "3" into an integer
    Serial.print("Received Mode: ");
    Serial.println(currentMode);

    // Immediate overrides for On/Off
    if (currentMode == 0) { digitalWrite(ledPin, LOW); ledState = false; }
    if (currentMode == 1) { digitalWrite(ledPin, HIGH); ledState = true; }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    handleWebSocketMessage(arg, data, len);
  }
}

// --- Main Setup ---
void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Start the WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.print("Connect to WiFi: "); Serial.println(ssid);
  Serial.print("Open Browser to: "); Serial.println(WiFi.softAPIP());

  // Attach the WebSocket listener
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Serve the HTML page embedded above
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.begin();
}

// --- Main Loop ---
void loop() {
  ws.cleanupClients(); // Keep the websocket memory clean

  // We CANNOT use delay() because it freezes the web server.
  // We must use millis() to check how much time has passed.
  if (currentMode == 2 || currentMode == 3) {
    unsigned long currentMillis = millis();
    int blinkSpeed = (currentMode == 2) ? 1000 : 100; // 1000ms for slow, 100ms for fast
    
    if (currentMillis - previousMillis >= blinkSpeed) {
      previousMillis = currentMillis;
      ledState = !ledState; // Toggle the LED state
      digitalWrite(ledPin, ledState);
    }
  }
}

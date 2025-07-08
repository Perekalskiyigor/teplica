#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <HTTPUpdateServer.h>

// --- Пины и конфигурации ---
#define PIN_R 25
#define PIN_G 26
#define PIN_B 27

#define DHTPIN 4
#define DHTTYPE DHT22

#define OTAUSER         "teplica321"
#define OTAPASSWORD     "Fnkfynblf!(*&14"
#define OTAPATH         "/firmware"
#define SERVERPORT      80

// --- Объекты ---
WebServer HttpServer(SERVERPORT);
HTTPUpdateServer httpUpdater;

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

// --- Настройки WiFi и MQTT ---
const char* ssid = "POCO X3 Pro";
const char* password = "Fnkfynblf1987";
const char* mqtt_server = "37.79.202.158";

// --- Пороговые значения ---
const float PEAK_TEMP = 65;
const float PEAK_HUM = 70;

// --- Переменные ---
enum Mode { AUTO, MANUAL };
Mode currentMode = AUTO;

unsigned long peakStart = 0;
bool waveActive = false;

int manualR = 0, manualG = 0, manualB = 0;

// --- Настройка PWM ---
void setupPWM() {
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
}

// --- Установка цвета ---
void setColor(int r, int g, int b) {
  analogWrite(PIN_R, r);
  analogWrite(PIN_G, g);
  analogWrite(PIN_B, b);
}

// --- Подключение к WiFi ---
void setup_wifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// --- MQTT callback ---
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  if (String(topic) == "relax/mode") {
    currentMode = (msg == "manual") ? MANUAL : AUTO;
  }
  if (String(topic) == "relax/color") {
    sscanf(msg.c_str(), "%d,%d,%d", &manualR, &manualG, &manualB);
  }
}

// --- Подключение к MQTT ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Relax")) {
      Serial.println("connected");
      client.subscribe("relax/mode");
      client.subscribe("relax/color");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  setupPWM();

  if (!MDNS.begin("esp32")) {
    Serial.println("Error setting up MDNS responder!");
  }

  httpUpdater.setup(&HttpServer, OTAPATH, OTAUSER, OTAPASSWORD);
  HttpServer.begin();
  Serial.println("HTTP server started");
}

// --- Главный цикл ---
void loop() {
  HttpServer.handleClient();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t)) {
    Serial.println("Failed to read temperature from DHT, using default 20°C");
    t = 20;
  }

  if (isnan(h)) {
    Serial.println("Failed to read humidity from DHT, using default 20%");
    h = 20;
  }

  static unsigned long lastCheck = 0;
  unsigned long now = millis();

  
    if (now - lastCheck > 2000) {
    lastCheck = now;
    client.publish("relax/temperature", String(t).c_str());
    client.publish("relax/humidity", String(h).c_str());

    // IP адрес как строка
  IPAddress ip = WiFi.localIP();
  String ipStr = ip.toString();
  client.publish("relax/ip", ipStr.c_str());

  // Уровень сигнала (RSSI)
  int rssi = WiFi.RSSI();
  client.publish("relax/rssi", String(rssi).c_str());

  // Лог в порт
  Serial.printf("Temp: %.1f °C | Humidity: %.1f %% | IP: %s | RSSI: %d dBm\n", t, h, ipStr.c_str(), rssi);

  }

  if (currentMode == MANUAL) {
    client.publish("relax/mode/out", "MANUAL");
    setColor(manualR, manualG, manualB);
    char colorMsg[16];
    snprintf(colorMsg, sizeof(colorMsg), "%d,%d,%d", manualR, manualG, manualB);
    client.publish("relax/color/out", colorMsg);
    return;
  }

  // --- Автоматический режим ---
  if (currentMode == AUTO) {
    client.publish("relax/mode/out", "AUTO");
    if (t > PEAK_TEMP && h > PEAK_HUM && !waveActive) {
      waveActive = true;
      peakStart = now;
    }

    if (waveActive) {
      unsigned long elapsed = now - peakStart;

      int r = 0, g = 0, b = 0;
      float phase = fmod(now / 1000.0, 5.0);
      float breath = (sin(phase * 2 * PI / 5.0) + 1.0) / 2.0 * 150 + 50;

      if (elapsed < 2 * 60 * 1000) {
        // Фаза 1: фиолетовая
        r = breath * 0.6;
        g = 0;
        b = breath;
      } else if (elapsed < 4 * 60 * 1000) {
        // Фаза 2: синяя
        r = 0;
        g = 0;
        b = breath;
      } else if (elapsed < 7 * 60 * 1000) {
        // Фаза 3: зелёная
        r = 0;
        g = breath;
        b = breath * 0.4;
      } else {
        waveActive = false;
        setColor(0, 0, 0);
      }

      setColor(r, g, b);
    }
  }
}

#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>

// Прошивка удаленно
#define OTAUSER         "teplica321"    // Логин для входа в OTA
#define OTAPASSWORD     "Fnkfynblf!(*&14"    // Пароль для входа в ОТА
#define OTAPATH         "/firmware"      // Путь, который будем дописывать после ip адреса в браузере.
#define SERVERPORT      80               // Порт для входа, он стандартный 80 это порт http
WebServer HttpServer(SERVERPORT);
HTTPUpdateServer httpUpdater;

// Определение пинов для ESP32 (не используем пины 0, 2, 12, 15, которые могут мешать загрузке)
#define Watering        14    // Полив
#define lighting       27    // Освещение
#define ventilation    26    // Вентиляция
#define heating        25    // Обогрев
#define water_filling  33    // Наполнение воды
#define SensorWater    32    // Датчик уровня воды (вход)
#define SystemWaterValve 13  // Клапан полива

// Флаги режимов
bool autoMode = false; // false - обычный режим, true - авторежим (управление с P-топиков)

// Глобальные переменные для контроля наполнения воды в бак
bool fillingActive = false;
unsigned long fillStartTime = 0;
const unsigned long FILL_TIMEOUT = 7200000; // 2 часа в миллисекундах

// Таймеры
unsigned long previousMillis = 0;  // Переменная для отслеживания времени
const long interval = 2000;        // Интервал 2 секунды

// Настройки WiFi и MQTT
const char* ssid = "POCO X3 Pro";
const char* password = "Fnkfynblf1987";
const char* mqtt_server = "37.79.202.158";
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Подключение к ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi подключено");
  Serial.println("IP адрес: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Получено сообщение [");
  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Реализован авто режим, когда мы его получаем, контроллер игнорирует сервер и полностью управляется с телефона
  // Обработка топика режима
  if (strcmp(topic, "teplica321/mode") == 0) {
    if ((char)payload[0] == '1') {
      autoMode = true;
      Serial.println("Активирован авторежим (управление через P-топики)");
      client.publish("teplica321/mode/status", "1");
    } else {
      autoMode = false;
      Serial.println("Обычный режим управления");
      client.publish("teplica321/mode/status", "0");
    }
    return;
  }

  // Обработка команд управления с учетом режима
  if (!autoMode) {
    ///////////////////// Полив ////////////////////////
    if (strcmp(topic, "teplica321/waterPump/in") == 0) { 
      if ((char)payload[0] == '0') {
        Serial.println("teplica321/waterPump/in 0");
        client.publish("teplica321/waterPump/out", "0"); 
        client.publish("teplica321/log", "Полив выключен");
        digitalWrite(Watering, HIGH); // Полив выключен (активный низкий сигнал)
      } 
      else if ((char)payload[0] == '1') {
        Serial.println("teplica321/waterPump/in 1");
        client.publish("teplica321/waterPump/out", "1");
        digitalWrite(Watering, LOW); // Полив включен (активный низкий сигнал)
        client.publish("teplica321/log", "Полив включен");
      } 
    }
      
    ///////////////////// Наполнение воды ////////////////////////
    if (strcmp(topic, "teplica321/waterValve/in") == 0) { 
      if ((char)payload[0] == '0') {
        Serial.println("teplica321/waterValve/in 0");
        client.publish("teplica321/waterValve/out", "0"); 
        digitalWrite(water_filling, HIGH); // Выключить наполнение воды (активный низкий сигнал)
        fillingActive = false;
        client.publish("teplica321/log", "Наполнение воды выключено P");
      } 
      else if ((char)payload[0] == '1') { 
        Serial.println("teplica321/waterValve/in 1");
        digitalWrite(water_filling, LOW); // Включить наполнение
        fillingActive = true;
        fillStartTime = millis();
        client.publish("teplica321/waterValve/out", "1");
        client.publish("teplica321/log", "Наполнение воды включено");
      } 
    }
    
    ///////////////////// Освещение ///////////////////////
    if (strcmp(topic, "teplica321/light/in") == 0) { 
      if ((char)payload[0] == '0') {
        Serial.println("teplica321/light/in 0");
        client.publish("teplica321/light/out", "0"); 
        digitalWrite(lighting, HIGH); // Выключить свет (активный низкий сигнал)
        client.publish("teplica321/log", "Свет выключен");
      } 
      else if ((char)payload[0] == '1') { 
        Serial.println("teplica321/light/in 1");
        client.publish("teplica321/light/out", "1");
        digitalWrite(lighting, LOW); // Включить свет (активный низкий сигнал)
        client.publish("teplica321/log", "Свет включен");
      } 
    }
    
    ///////////////////// Вентиляция ///////////////////////
    if (strcmp(topic, "teplica321/fan/in") == 0) { 
      if ((char)payload[0] == '0') {
        Serial.println("teplica321/fan/in 0");
        client.publish("teplica321/fan/out", "0"); 
        digitalWrite(ventilation, HIGH); // Выключить вентиляцию (активный низкий сигнал)
        client.publish("teplica321/log", "Вентилятор выключен");
      } 
      else if ((char)payload[0] == '1') { 
        Serial.println("teplica321/fan/in 1");
        client.publish("teplica321/fan/out", "1");
        digitalWrite(ventilation, LOW); // Включить вентиляцию (активный низкий сигнал)
        client.publish("teplica321/log", "Вентилятор включен");
      } 
    }
    
    ///////////////////// Отопление ///////////////////////
    if (strcmp(topic, "teplica321/hot/in") == 0) { 
      if ((char)payload[0] == '0') {
        Serial.println("teplica321/hot/in 0");
        client.publish("teplica321/hot/out", "0"); 
        digitalWrite(heating, HIGH); // Выключить отопление (активный низкий сигнал)
        client.publish("teplica321/log", "Отопление выключено");
      } 
      else if ((char)payload[0] == '1') { 
        Serial.println("teplica321/hot/in 1");
        client.publish("teplica321/hot/out", "1");
        digitalWrite(heating, LOW); // Включить отопление (активный низкий сигнал)
        client.publish("teplica321/log", "Отопление включено");
      } 
    }

    ///////////////////// Полив огорода ///////////////////////
    if (strcmp(topic, "teplica321/Valve1/in") == 0) { 
      if ((char)payload[0] == '0') {
        Serial.println("teplica321/Valve1/in 0");
        client.publish("teplica321/Valve1/out", "0"); 
        digitalWrite(SystemWaterValve, LOW); // Выключить полив (активный низкий сигнал)
        client.publish("teplica321/log", "Полив огорода выключено");
      } 
      else if ((char)payload[0] == '1') { 
        Serial.println("teplica321/Valve1/in 1");
        client.publish("teplica321/Valve1/out", "1");
        digitalWrite(SystemWaterValve, HIGH); // Включить полив огорода (активный низкий сигнал)
        client.publish("teplica321/log", "Полив огорода включено");
      } 
    }
  } else {
    // Авторежим (управление через P-топики)
    ///////////////////// Полив ////////////////////////
    if (strcmp(topic, "teplica321/PwaterPump/in") == 0) { 
      if ((char)payload[0] == '0') {
        Serial.println("teplica321/PwaterPump/in 0");
        client.publish("teplica321/waterPump/out", "0"); 
        client.publish("teplica321/log", "P Полив выключен");
        digitalWrite(Watering, HIGH); // Полив выключен (активный низкий сигнал)
      } 
      else if ((char)payload[0] == '1') {
        Serial.println("teplica321/PwaterPump/in 1");
        client.publish("teplica321/waterPump/out", "1");
        digitalWrite(Watering, LOW); // Полив включен (активный низкий сигнал)
        client.publish("teplica321/log", "Полив включен P");
      } 
    }
      
    ///////////////////// Наполнение воды ////////////////////////
    if (strcmp(topic, "teplica321/PwaterValve/in") == 0) { 
      if ((char)payload[0] == '0') {
        Serial.println("teplica321/PwaterValve/in 0");
        client.publish("teplica321/waterValve/out", "0"); 
        digitalWrite(water_filling, HIGH); // Выключить наполнение воды (активный низкий сигнал)
        fillingActive = false;
        client.publish("teplica321/log", "Наполнение воды выключено P");
      } 
      else if ((char)payload[0] == '1') { 
        Serial.println("teplica321/PwaterValve/in 1");
        digitalWrite(water_filling, LOW); // Включить наполнение
        fillingActive = true;
        fillStartTime = millis();
        client.publish("teplica321/waterValve/out", "1");
        client.publish("teplica321/log", "Наполнение воды включено P");
      } 
    }
    
    ///////////////////// Освещение ///////////////////////
    if (strcmp(topic, "teplica321/Plight/in") == 0) { 
      if ((char)payload[0] == '0') {
        Serial.println("teplica321/Plight/in 0");
        client.publish("teplica321/light/out", "0"); 
        digitalWrite(lighting, HIGH); // Выключить свет (активный низкий сигнал)
        client.publish("teplica321/log", "Свет выключен P");
      } 
      else if ((char)payload[0] == '1') { 
        Serial.println("teplica321/Plight/in 1");
        client.publish("teplica321/light/out", "1");
        digitalWrite(lighting, LOW); // Включить свет (активный низкий сигнал)
        client.publish("teplica321/log", "Свет включен P");
      } 
    }
    
    ///////////////////// Вентиляция ///////////////////////
    if (strcmp(topic, "teplica321/Pfan/in") == 0) { 
      if ((char)payload[0] == '0') {
        Serial.println("teplica321/Pfan/in 0");
        client.publish("teplica321/fan/out", "0"); 
        digitalWrite(ventilation, HIGH); // Выключить вентиляцию (активный низкий сигнал)
        client.publish("teplica321/log", "Вентилятор выключен P");
      } 
      else if ((char)payload[0] == '1') { 
        Serial.println("teplica321/Pfan/in 1");
        client.publish("teplica321/fan/out", "1");
        digitalWrite(ventilation, LOW); // Включить вентиляцию (активный низкий сигнал)
        client.publish("teplica321/log", "Вентилятор включен P");
      } 
    }
    
    ///////////////////// Отопление ///////////////////////
    if (strcmp(topic, "teplica321/Phot/in") == 0) { 
      if ((char)payload[0] == '0') {
        Serial.println("teplica321/Phot/in 0");
        client.publish("teplica321/hot/out", "0"); 
        digitalWrite(heating, HIGH); // Выключить отопление (активный низкий сигнал)
        client.publish("teplica321/log", "Отопление выключено P");
      } 
      else if ((char)payload[0] == '1') { 
        Serial.println("teplica321/Phot/in 1");
        client.publish("teplica321/hot/out", "1");
        digitalWrite(heating, LOW); // Включить отопление (активный низкий сигнал)
        client.publish("teplica321/log", "Отопление включено P");
      } 
    }

    ///////////////////// Полив огорода ///////////////////////
    if (strcmp(topic, "teplica321/PValve1/in") == 0) { 
      if ((char)payload[0] == '0') {
        Serial.println("teplica321/PValve1/in 0");
        client.publish("teplica321/Valve1/out", "0"); 
        digitalWrite(SystemWaterValve, LOW); // Выключить полив (активный низкий сигнал)
        client.publish("teplica321/log", "Полив огорода выключено");
      } 
      else if ((char)payload[0] == '1') { 
        Serial.println("teplica321/PValve1/in 1");
        client.publish("teplica321/Valve1/out", "1");
        digitalWrite(SystemWaterValve, HIGH); // Включить полив огорода (активный низкий сигнал)
        client.publish("teplica321/log", "Полив огорода включено");
      } 
    }
  }
}

unsigned long nextConnectionAttempt = 0;

void reconnect() {
  while (!client.connected()) {
    unsigned long now = millis();
    if (now >= nextConnectionAttempt) {
      Serial.print("Попытка подключения к MQTT...");
      String clientId = "ESP32Client-";
      clientId += String(random(0xffff), HEX);
      if (client.connect(clientId.c_str())) {
        Serial.println("подключено");
        
        // Подписываемся на топики
        client.subscribe("teplica321/mode");
        
        // Подписки для обычного режима
        client.subscribe("teplica321/waterPump/in");
        client.subscribe("teplica321/waterValve/in");
        client.subscribe("teplica321/light/in");
        client.subscribe("teplica321/fan/in");
        client.subscribe("teplica321/hot/in");
        client.subscribe("teplica321/Valve1/in");

        // Подписки для авторежима
        client.subscribe("teplica321/PwaterPump/in");
        client.subscribe("teplica321/PwaterValve/in");
        client.subscribe("teplica321/Plight/in");
        client.subscribe("teplica321/Pfan/in");
        client.subscribe("teplica321/Phot/in");
        client.subscribe("teplica321/PValve1/in");
        
        nextConnectionAttempt = millis() + 5000; // Следующая попытка через 5 секунд
      } else {
        Serial.print("Не удалось подключиться, rc=");
        Serial.print(client.state());
        Serial.println(" попробовать снова через 5 секунд");
        nextConnectionAttempt = millis() + 5000; // Следующая попытка через 5 секунд
      }
    }
    delay(100); // Меньший интервал для более плавного выполнения
  }
}

void setup() {

  // Настройка пинов
  pinMode(Watering, OUTPUT);
  pinMode(lighting, OUTPUT); 
  pinMode(ventilation, OUTPUT);
  pinMode(heating, OUTPUT);
  pinMode(water_filling, OUTPUT);
  pinMode(SensorWater, INPUT_PULLUP); 
  pinMode(SystemWaterValve, OUTPUT);

  // Инициализация всех реле в выключенное состояние
  digitalWrite(Watering, HIGH);
  digitalWrite(lighting, HIGH);
  digitalWrite(ventilation, HIGH);
  digitalWrite(heating, HIGH);
  digitalWrite(water_filling, HIGH);
  digitalWrite(SystemWaterValve, LOW);
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Инициализация OTA
  httpUpdater.setup(&HttpServer, OTAPATH, OTAUSER, OTAPASSWORD);
  HttpServer.onNotFound(handleNotFound);
  HttpServer.begin();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  

  // Проверяем соединение с Wi-Fi каждые 10 секунд и публикуем уровень сигнала
  if (millis() % 10000 == 0) { // Проверяем каждые 10 секунд
    Serial.println(millis());
    int signalLevel = WiFi.RSSI(); // Получаем уровень сигнала Wi-Fi
    Serial.print("Wi-Fi Signal Level: ");
    Serial.println(signalLevel);
    client.publish("teplica321/signal", String(signalLevel).c_str()); // Публикуем уровень сигнала
    
    // Преобразование IP-адреса в строку
    IPAddress ip = WiFi.localIP();
    String ipAddress = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);

    // Публикация IP-адреса в MQTT-топике
    client.publish("teplica321/ipAddress", ipAddress.c_str());

    client.publish("teplica321/Version", "1.0");
  }

  // Каждые 2 секунды публикуем состояние датчика воды
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // Читаем значение с датчика
    int relayLevel = digitalRead(SensorWater);
    
    // Выводим для отладки
    Serial.print("SensorWater: ");
    Serial.println(relayLevel);
    
    // Публикуем состояние датчика
    client.publish("teplica321/SensorWater", String(relayLevel).c_str());
  }

  // Проверка датчика воды и таймаута наполнения для 2 режимов
  if (fillingActive) {
    if (digitalRead(SensorWater) == 1) {
      digitalWrite(water_filling, HIGH);
      fillingActive = false;
      String outTopic = autoMode ? "teplica321/PwaterValve/out" : "teplica321/waterValve/out";
      client.publish(outTopic.c_str(), "0");
      client.publish("teplica321/log", autoMode ? 
          "Автоотключение: вода достигла уровня P режим" : 
          "Автоотключение: вода достигла уровня");
    }
    else if (millis() - fillStartTime >= FILL_TIMEOUT) {
      digitalWrite(water_filling, HIGH);
      fillingActive = false;
      String outTopic = autoMode ? "teplica321/PwaterValve/out" : "teplica321/waterValve/out";
      client.publish(outTopic.c_str(), "0");
      client.publish("teplica321/log", autoMode ? 
          "Автоотключение: сработал таймаут 2 часа P режим" : 
          "Автоотключение: сработал таймаут 2 часа");
    }
  }
  HttpServer.handleClient(); // Прослушивание HTTP-запросов от клиентов
}

// Для удаленной прошивки
/* Выводить надпись, если такой страницы ненайдено */
// Обработчик для несуществующих страниц
void handleNotFound() {
  String message = "404: Not found\n\n";
  message += "URI: ";
  message += HttpServer.uri();
  message += "\nMethod: ";
  message += (HttpServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += HttpServer.args();
  message += "\n";
  
  for (uint8_t i = 0; i < HttpServer.args(); i++) {
    message += " " + HttpServer.argName(i) + ": " + HttpServer.arg(i) + "\n";
  }
  
  HttpServer.send(404, "text/plain", message);
}

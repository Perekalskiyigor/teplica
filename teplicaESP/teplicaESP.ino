#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

//прошивка удаленно
#define OTAUSER         "teplica"    // Логин для входа в OTA
#define OTAPASSWORD     "Fnkfynblf!(*&14"    // Пароль для входа в ОТА
#define OTAPATH         "/firmware"// Путь, который будем дописывать после ip адреса в браузере.
#define SERVERPORT      80         // Порт для входа, он стандартный 80 это порт http
ESP8266WebServer HttpServer(SERVERPORT);
ESP8266HTTPUpdateServer httpUpdater;



// Определение пинов
#define Watering D2    //Полив
#define lighting D3    //Освещение
#define ventilation D4 //Вентиляция
#define heating D5     // Обогрев
#define water_filling D6 //Наполнение воды
#define SensorWater D7 //Датчик уровня воды

// Флаги режимов
bool autoMode = false; // false - обычный режим, true - авторежим (управление с P-топиков)

// Глобальные переменные для контроля наполнения воды в бак
bool fillingActive = false;
unsigned long fillStartTime = 0;
const unsigned long FILL_TIMEOUT = 7200000; // 2 часа в миллисекундах


// Таймеры
unsigned long lastRestartTime = 0; // Время последней перезагрузки
const unsigned long RESTART_INTERVAL = 7200000; // Интервал 2 часа в миллисекундах
unsigned long previousMillis = 0;  // Переменная для отслеживания времени
const long interval = 2000;         // Интервал 5 секунд


// Настройки WiFi и MQTT
const char* ssid = "UFSB";
const char* password = "Fnkfynblf!(*&14";
const char* mqtt_server = "37.79.202.158";
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Подключение к ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status()!= WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi подключено");
  Serial.println("IP адрес: ");
  Serial.println(WiFi.localIP());
}



/////////////////////Функция обработки топиков/////////////////////
//Не забудь подписаться на топик ниже client.subscribe("teplica/hardoff");

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Получено сообщение [");
  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  
  //Реализован авто режим, когда мы его получаем, контроллер игнрирует сервер и полностью управляется с телефона
  // Обработка топика режима
  if (strcmp(topic, "teplica/mode") == 0) {
    if ((char)payload[0] == '1') {
      autoMode = true;
      Serial.println("Активирован авторежим (управление через P-топики)");
      client.publish("teplica/mode/status", "1");
    } else {
      autoMode = false;
      Serial.println("Обычный режим управления");
      client.publish("teplica/mode/status", "0");
    }
    return;
  }

    // Обработка команд управления с учетом режима
  if (!autoMode) {
    
        ///////////////////// Полив ////////////////////////
        if (strcmp(topic, "teplica/waterPump/in") == 0) { 
          if ((char)payload[0] == '0') {
            Serial.println("teplica/waterPump/in 0");
            client.publish("teplica/waterPump/out", "0"); 
            client.publish("teplica/log", "Полив выключен");
            // Здесь код для реле на отключение
            digitalWrite(Watering, HIGH); // Полив выключен (активный низкий сигнал)
          } 
          else if ((char)payload[0] == '1') {
            Serial.println("teplica/waterPump/in 1");
            client.publish("teplica/waterPump/out", "1");
            digitalWrite(Watering, LOW); // Полив включен (активный низкий сигнал)
            client.publish("teplica/log", "Полив включен");
          } 
        }
          
        ///////////////////// Наполнение воды ////////////////////////
        if (strcmp(topic, "teplica/waterValve/in") == 0) { 
          if ((char)payload[0] == '0') {
            Serial.println("teplica/waterValve/in 0");
            client.publish("teplica/waterValve/out", "0"); 
            digitalWrite(water_filling, HIGH); // Выключить наполнение воды (активный низкий сигнал)
            fillingActive = false;
            client.publish("teplica/log", "Наполнение воды выключено P");
          } 
          else if ((char)payload[0] == '1') { 
            Serial.println("teplica/waterValve/in 1");
            digitalWrite(water_filling, LOW); // Включить наполнение
            fillingActive = true;
            fillStartTime = millis();
            client.publish("teplica/waterValve/out", "1");
            client.publish("teplica/log", "Наполнение воды включено");
          } 
        }
        
        ///////////////////// Освещение ///////////////////////
        if (strcmp(topic, "teplica/light/in") == 0) { 
          if ((char)payload[0] == '0') {
            Serial.println("teplica/light/in 0");
            client.publish("teplica/light/out", "0"); 
            digitalWrite(lighting, HIGH); // Выключить свет (активный низкий сигнал)
            client.publish("teplica/log", "Свет выключен");
          } 
          else if ((char)payload[0] == '1') { 
            Serial.println("teplica/light/in 1");
            client.publish("teplica/light/out", "1");
            digitalWrite(lighting, LOW); // Включить свет (активный низкий сигнал)
            client.publish("teplica/log", "Свет включен");
          } 
        }
        
        ///////////////////// Вентиляция ///////////////////////
        if (strcmp(topic, "teplica/fan/in") == 0) { 
          if ((char)payload[0] == '0') {
            Serial.println("teplica/fan/in 0");
            client.publish("teplica/fan/out", "0"); 
            digitalWrite(ventilation, HIGH); // Выключить вентиляцию (активный низкий сигнал)
            client.publish("teplica/log", "Вентилятор выключен");
          } 
          else if ((char)payload[0] == '1') { 
            Serial.println("teplica/fan/in 1");
            client.publish("teplica/fan/out", "1");
            digitalWrite(ventilation, LOW); // Включить вентиляцию (активный низкий сигнал)
            client.publish("teplica/log", "Вентилятор включен");
          } 
        }
        
        ///////////////////// Отопление ///////////////////////
        if (strcmp(topic, "teplica/hot/in") == 0) { 
          if ((char)payload[0] == '0') {
            Serial.println("teplica/hot/in 0");
            client.publish("teplica/hot/out", "0"); 
            digitalWrite(heating, HIGH); // Выключить отопление (активный низкий сигнал)
            client.publish("teplica/log", "Отопление выключено");
          } 
          else if ((char)payload[0] == '1') { 
            Serial.println("teplica/hot/in 1");
            client.publish("teplica/hot/out", "1");
            digitalWrite(heating, LOW); // Включить отопление (активный низкий сигнал)
            client.publish("teplica/log", "Отопление включено");
          } 
        }
        } else {
        //////////////////////////////////////////////////////////////////////////////////////////
        // Авторежим (управление через P-топики)
        /////////////////////////////////////////////////////////////////////////////////////////
        
        ///////////////////// Полив ////////////////////////
        if (strcmp(topic, "teplica/PwaterPump/in") == 0) { 
          if ((char)payload[0] == '0') {
            Serial.println("teplica/PwaterPump/in 0");
            client.publish("teplica/waterPump/out", "0"); 
            client.publish("teplica/log", "P Полив выключен");
            // Здесь код для реле на отключение
            digitalWrite(Watering, HIGH); // Полив выключен (активный низкий сигнал)
          } 
          else if ((char)payload[0] == '1') {
            Serial.println("teplica/PwaterPump/in 1");
            client.publish("teplica/waterPump/out", "1");
            digitalWrite(Watering, LOW); // Полив включен (активный низкий сигнал)
            client.publish("teplica/log", "Полив включен P");
          } 
        }
          
        ///////////////////// Наполнение воды ////////////////////////
        if (strcmp(topic, "teplica/PwaterValve/in") == 0) { 
          if ((char)payload[0] == '0') {
            Serial.println("teplica/PwaterValve/in 0");
            client.publish("teplica/waterValve/out", "0"); 
            digitalWrite(water_filling, HIGH); // Выключить наполнение воды (активный низкий сигнал)
            fillingActive = false;
            client.publish("teplica/log", "Наполнение воды выключено P");
          } 
          else if ((char)payload[0] == '1') { 
            Serial.println("teplica/PwaterValve/in 1");
            digitalWrite(water_filling, LOW); // Включить наполнение
            fillingActive = true;
            fillStartTime = millis();
            client.publish("teplica/waterValve/out", "1");
            client.publish("teplica/log", "Наполнение воды включено P");
          } 
        }
        
        ///////////////////// Освещение ///////////////////////
        if (strcmp(topic, "teplica/Plight/in") == 0) { 
          if ((char)payload[0] == '0') {
            Serial.println("teplica/Plight/in 0");
            client.publish("teplica/light/out", "0"); 
            digitalWrite(lighting, HIGH); // Выключить свет (активный низкий сигнал)
            client.publish("teplica/log", "Свет выключен P");
          } 
          else if ((char)payload[0] == '1') { 
            Serial.println("teplica/Plight/in 1");
            client.publish("teplica/light/out", "1");
            digitalWrite(lighting, LOW); // Включить свет (активный низкий сигнал)
            client.publish("teplica/log", "Свет включен P");
          } 
        }
        
        ///////////////////// Вентиляция ///////////////////////
        if (strcmp(topic, "teplica/Pfan/in") == 0) { 
          if ((char)payload[0] == '0') {
            Serial.println("teplica/Pfan/in 0");
            client.publish("teplica/fan/out", "0"); 
            digitalWrite(ventilation, HIGH); // Выключить вентиляцию (активный низкий сигнал)
            client.publish("teplica/log", "Вентилятор выключен P");
          } 
          else if ((char)payload[0] == '1') { 
            Serial.println("teplica/Pfan/in 1");
            client.publish("teplica/fan/out", "1");
            digitalWrite(ventilation, LOW); // Включить вентиляцию (активный низкий сигнал)
            client.publish("teplica/log", "Вентилятор включен P");
          } 
        }
        
        ///////////////////// Отопление ///////////////////////
        if (strcmp(topic, "teplica/Phot/in") == 0) { 
          if ((char)payload[0] == '0') {
            Serial.println("teplica/Phot/in 0");
            client.publish("teplica/hot/out", "0"); 
            digitalWrite(heating, HIGH); // Выключить отопление (активный низкий сигнал)
            client.publish("teplica/log", "Отопление выключено P");
          } 
          else if ((char)payload[0] == '1') { 
            Serial.println("teplica/Phot/in 1");
            client.publish("teplica/hot/out", "1");
            digitalWrite(heating, LOW); // Включить отопление (активный низкий сигнал)
            client.publish("teplica/log", "Отопление включено P");
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
          String clientId = "ESP8266Client-";
          clientId += String(random(0xffff), HEX);
          if (client.connect(clientId.c_str())) {
            Serial.println("подключено");
            
            // /////////////////Подписываемся на топик//////////////////////////
            client.subscribe("teplica/mode");
            
            // Подписки для обычного режима
            client.subscribe("teplica/waterPump/in");
            client.subscribe("teplica/waterValve/in");
            client.subscribe("teplica/light/in");
            client.subscribe("teplica/fan/in");
            client.subscribe("teplica/hot/in");

            // Подписки для авторежима
            client.subscribe("teplica/PwaterPump/in");
            client.subscribe("teplica/PwaterValve/in");
            client.subscribe("teplica/Plight/in");
            client.subscribe("teplica/Pfan/in");
            client.subscribe("teplica/Phot/in");
            
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

  //Прошивка удаленно
  // Настройка OTA
  httpUpdater.setup(&HttpServer, OTAPATH, OTAUSER, OTAPASSWORD);
  HttpServer.onNotFound(handleNotFound);
  HttpServer.begin();

  // Настройка пинов
  pinMode(Watering, OUTPUT);
  pinMode(lighting, OUTPUT); 
  pinMode(ventilation, OUTPUT);
  pinMode(heating, OUTPUT);
  pinMode(water_filling, OUTPUT);
  pinMode(SensorWater, INPUT_PULLUP); 

  // Инициализация всех реле в выключенное состояние
  digitalWrite(Watering, HIGH);
  digitalWrite(lighting, HIGH);
  digitalWrite(ventilation, HIGH);
  digitalWrite(heating, HIGH);
  digitalWrite(water_filling, HIGH);
   
 
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

   // Настройка Watchdog Timer на 30 секунд
  ESP.wdtEnable(30000);
  
  // Проверяем, не было ли перезапуска из-за зависания
  if (ESP.getResetReason() == "Software Watchdog") {
    Serial.println("Перезапуск из-за зависания!");
    client.publish("teplica/log", "Watchdog timer activate");
    // Здесь можно добавить дополнительную логику при перезапуске
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  HttpServer.handleClient();       // Прослушивание HTTP-запросов от клиентов

  // Обновляем Watchdog Timer
  ESP.wdtFeed();

  // Проверяем соединение с Wi-Fi каждые 10 секунд и публикуем уровень сигнала
  if (millis() % 10000 == 0) { // Проверяем каждые 10 секунд
    Serial.println(millis());
    int signalLevel = WiFi.RSSI(); // Получаем уровень сигнала Wi-Fi
    Serial.print("Wi-Fi Signal Level: ");
    Serial.println(signalLevel);
    client.publish("teplica/signal", String(signalLevel).c_str()); // Публикуем уровень сигнала
    
    // Преобразование IP-адреса в строку
    IPAddress ip = WiFi.localIP();
    String ipAddress = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);

    // Публикация IP-адреса в MQTT-топике
    client.publish("teplica/ipAddress", ipAddress.c_str());

    client.publish("teplica/Version", "1.0");
  }


    // Каждые 5 секунд и публикуем состояние датчика воды
    unsigned long currentMillis = millis();
    
    // Проверяем, прошло ли 5 секунд
    if (currentMillis - previousMillis >= interval) {
      // Сохраняем время последней публикации
      previousMillis = currentMillis;
      
      // Читаем значение с датчика
      int relayLevel = digitalRead(SensorWater);
      
      // Выводим для отладки
      Serial.print("SensorWater: ");
      Serial.println(relayLevel);
      
      // Публикуем состояние датчика
      client.publish("teplica/SensorWater", String(relayLevel).c_str());
    }
  
    // Проверка на необходимость перезагрузки каждые 2 часа
    if (millis() - lastRestartTime >= RESTART_INTERVAL) {
    Serial.println("Автоматическая перезагрузка через 2 часа");
    client.publish("teplica/log", "Автоматическая перезагрузка через 2 часа");
    ESP.restart(); // Перезагрузка контроллера
  }

  //// Проверка датчика воды и таймаута наполнения для 2 режимов
      if (fillingActive) {
        if (digitalRead(SensorWater) == 1) {
            digitalWrite(water_filling, HIGH);
            fillingActive = false;
            String outTopic = autoMode ? "teplica/PwaterValve/out" : "teplica/waterValve/out";
            client.publish(outTopic.c_str(), "0");
            client.publish("teplica/log", autoMode ? 
                "Автоотключение: вода достигла уровня P режим" : 
                "Автоотключение: вода достигла уровня");
        }
        else if (millis() - fillStartTime >= FILL_TIMEOUT) {
            digitalWrite(water_filling, HIGH);
            fillingActive = false;
            String outTopic = autoMode ? "teplica/PwaterValve/out" : "teplica/waterValve/out";
            client.publish(outTopic.c_str(), "0");
            client.publish("teplica/log", autoMode ? 
                "Автоотключение: сработал таймаут 2 часа P режим" : 
                "Автоотключение: сработал таймаут 2 часа");
        }
    }
}

//Для удаленной прошивки
/* Выводить надпись, если такой страницы ненайдено */
void handleNotFound() {
  HttpServer.send(404, "text/plain", "404: Not found");
}

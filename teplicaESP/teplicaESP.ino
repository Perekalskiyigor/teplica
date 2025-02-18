#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

//прошивка удаленно
#define OTAUSER         "zavod"    // Логин для входа в OTA
#define OTAPASSWORD     "Maks+100500"    // Пароль для входа в ОТА
#define OTAPATH         "/firmware"// Путь, который будем дописывать после ip адреса в браузере.
#define SERVERPORT      80         // Порт для входа, он стандартный 80 это порт http
ESP8266WebServer HttpServer(SERVERPORT);
ESP8266HTTPUpdateServer httpUpdater;



// Определение пинов
#define Buzzer_PIN D4
#define Relay_PIN D5


// Глобальная переменная для отслеживания состояния режима hardoff
bool hardOffModeActive = true; 

// Определение глобальной переменной в начало. Таймер рестарта 2 часа
unsigned long lastRestartTime = 0; // Время последней перезагрузки
const unsigned long RESTART_INTERVAL = 7200000; // Интервал 2 часа в миллисекундах


// Данные для подключения к Wi-Fi
const char* ssid = "POCO X3 Pro";
const char* password = "Fnkfynblf1987";
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

  
    ////////////////////////// Полив//////////////////////////
  if (strcmp(topic, "teplica/waterPump/in") == 0) { 
    // Проверяем, не активен ли режим hardoff. Важно если активен 
    //все остальное рубится и уходит в No action

    
          if ((char)payload[0] == '0') {
          // digitalWrite(Buzzer_PIN, HIGH); // Зуммер
          //Выключить полив
          Serial.println("teplica/waterPump/in 0");
          client.publish("teplica/waterPump/out", "0"); 
          // Здесь код для реле на отключение
        } 
        else if ((char)payload[0] == '1') { 
          //Включить полив
          Serial.println("teplica/waterPump/in 1");
          client.publish("teplica/waterPump/out", "1");
        } 
        else { // Блок else без условия
          //Serial.println("No action for waterPump");
        }
  }
  
  
  ///////////////////////Наолнение воды///////////////////////////////////
  if (strcmp(topic, "teplica/waterPump/in") == 0) { 
    // Проверяем, не активен ли режим hardoff. Важно если активен 
    //все остальное рубится и уходит в No action

    
          if ((char)payload[0] == '0') {
          // digitalWrite(Buzzer_PIN, HIGH); // Зуммер
          //Выключить наполнение воды
          Serial.println("teplica/waterValve/in 0");
          client.publish("teplica/waterValve/out", "0"); 
          // Здесь код для реле на отключение
        } 
        else if ((char)payload[0] == '1') { // Добавьте условие для Mode 1
          //Включить наполнение воды
          Serial.println("teplica/waterPump/in 1");
          client.publish("teplica/waterPump/out", "1");
        } 
        else { // Блок else без условия
          //Serial.println("No action for waterPump");
        }
  }

 ///////////////////////#Освещение///////////////////////////////////
  if (strcmp(topic, "teplica/light/in") == 0) { 
    // Проверяем, не активен ли режим hardoff. Важно если активен 
    //все остальное рубится и уходит в No action

    
          if ((char)payload[0] == '0') {
          // digitalWrite(Buzzer_PIN, HIGH); // Зуммер
          //Выключить свет
          Serial.println("teplica/light/in 0");
          client.publish("teplica/light/out", "0"); 
          // Здесь код для реле на отключение
        } 
        else if ((char)payload[0] == '1') { // Добавьте условие для Mode 1
          //Включить свет
          Serial.println("teplica/light/in 1");
          client.publish("teplica/light/out", "1");
        } 
        else { // Блок else без условия
          //Serial.println("No action for waterPump");
        }
  }


  ///////////////////////#Вентиляция///////////////////////////////////
  if (strcmp(topic, "teplica/light/in") == 0) { 
    // Проверяем, не активен ли режим hardoff. Важно если активен 
    //все остальное рубится и уходит в No action

   
          if ((char)payload[0] == '0') {
          // digitalWrite(Buzzer_PIN, HIGH); // Зуммер
          //Выключить вентиляцию
          Serial.println("teplica/fan/in 0");
          client.publish("teplica/fan/out", "0"); 
          // Здесь код для реле на отключение
        } 
        else if ((char)payload[0] == '1') { // Добавьте условие для Mode 1
          //Выключить наполнение воды
          Serial.println("teplica/fan/in 1");
          client.publish("teplica/light/out", "1");
        } 
        else { // Блок else без условия
          //Serial.println("No action for waterPump");
        }
  }



 ///////////////////////#Отопление///////////////////////////////////
  if (strcmp(topic, "teplica/light/in") == 0) { 
    // Проверяем, не активен ли режим hardoff. Важно если активен 
    //все остальное рубится и уходит в No action

    //Полив режимы
          if ((char)payload[0] == '0') {
          // digitalWrite(Buzzer_PIN, HIGH); // Зуммер
          //Выключить Отопление
          Serial.println("teplica/hot/in 0");
          client.publish("teplica/hot/out", "0"); 
          // Здесь код для реле на отключение
        } 
        else if ((char)payload[0] == '1') { // Добавьте условие для Mode 1
          //Включить Отопление
          Serial.println("teplica/hot/in 1");
          client.publish("teplica/hot/out", "1");
        } 
        else { // Блок else без условия
          //Serial.println("No action for waterPump");
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
        client.subscribe("teplica/hardoff"); 
        client.subscribe("teplica/waterPump/in");
        client.subscribe("teplica/waterValve/in");
        client.subscribe("teplica/light/in");
        client.subscribe("teplica/fan/in");
        client.subscribe("teplica/hot/in");
        
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
  httpUpdater.setup(&HttpServer, OTAPATH, OTAUSER, OTAPASSWORD);
  HttpServer.onNotFound(handleNotFound);
  HttpServer.begin();

  //Пины выхода
  pinMode(Buzzer_PIN, OUTPUT); //Зуммер
  digitalWrite(Buzzer_PIN, HIGH); // Зуммер тихо
  pinMode(Relay_PIN, INPUT); //Реле


  
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

    client.publish("teplica/Version", "2.3");
  }


  // Каждые 5 секунд и публикуем состояние датчика воды
  if (millis() % 5000 == 0) { // Проверяем каждые 10 секун
    int relayLevel = digitalRead(Relay_PIN);
    Serial.print("relay: ");
    Serial.println(relayLevel);
    client.publish("teplica/Relay", String(relayLevel).c_str()); // Публикуем состояние реле
  }
  
    // Проверка на необходимость перезагрузки каждые 4 часа
    if (millis() - lastRestartTime >= RESTART_INTERVAL) {
    Serial.println("Автоматическая перезагрузка через 4 часа");
    client.publish("teplica/log", "Auto reboot after 4 hours");
    ESP.restart(); // Перезагрузка контроллера
  }
}

//Для удаленной прошивки
/* Выводить надпись, если такой страницы ненайдено */
void handleNotFound() {
  HttpServer.send(404, "text/plain", "404: Not found");
}

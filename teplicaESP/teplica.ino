#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

//прошивка удаленно
#define OTAUSER         "Moriarty"    // Логин для входа в OTA
#define OTAPASSWORD     "SherlockHolms"    // Пароль для входа в ОТА
#define OTAPATH         "/firmware"// Путь, который будем дописывать после ip адреса в браузере.
#define SERVERPORT      80         // Порт для входа, он стандартный 80 это порт http
ESP8266WebServer HttpServer(SERVERPORT);
ESP8266HTTPUpdateServer httpUpdater;



// Определение пинов
#define waterPump D3
#define waterValve D4
#define light D5
#define fan D6
#define hot D7
#define Buzzer_PIN D8


// Глобальная переменная для отслеживания состояния режима hardoff
bool hardOffModeActive = false; 

// Определение глобальной переменной в начало. Таймер рестарта 4 часа
unsigned long lastRestartTime = 0; // Время последней перезагрузки
const unsigned long RESTART_INTERVAL = 14400000; // Интервал 4 часа в миллисекундах


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

//Функция обработки топиков
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Получено сообщение [");
  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  
    // Проверяем, что сообщение пришло, и не включен режим жесткого отключения
    //Полив
    if (strcmp(topic, "teplica/waterPump") == 0 && !hardOffModeActive) {
      if ((char)payload[0] == '1') { 
          digitalWrite(waterPump, HIGH); // Включай полив
          Serial.println("waterPump activated");
          client.publish("teplica/waterPumpOut", "1"); // Режим принят
      } else if ((char)payload[0] == '0') {
        digitalWrite(waterPump, LOW);
        Serial.println("waterPump deactivated");
        client.publish("teplica/waterPumpOut", "0");
    }
        
    //Набор воды
    } else if (strcmp(topic, "teplica/waterValve") == 0 && !hardOffModeActive) {
        if ((char)payload[0] == '1') { 
            digitalWrite(waterValve, HIGH); 
            Serial.println("waterValve activated");
            client.publish("teplica/waterValvefOut", "1");
        } else if ((char)payload[0] == '0') {
            digitalWrite(waterValve, LOW); 
            Serial.println("waterValve deactivated");
            client.publish("teplica/waterValvefOut", "0");
        }

        
    //Освещение
    } else if (strcmp(topic, "teplica/light") == 0 && !hardOffModeActive) {
        if ((char)payload[0] == '1') { 
            digitalWrite(light, HIGH); 
            Serial.println("light activated");
            client.publish("teplica/lightOut", "1");
       } else if ((char)payload[0] == '0') {
            digitalWrite(light, LOW); 
            Serial.println("light deactivated");
            client.publish("teplica/lightOut", "0");
       }
    
    //Вентиляция
    } else if (strcmp(topic, "teplica/fan") == 0 && !hardOffModeActive) {
        if ((char)payload[0] == '1') { 
            digitalWrite(fan, HIGH); 
            Serial.println("fan activated");
            client.publish("teplica/fanOut", "1");
        } else if ((char)payload[0] == '0') {
            digitalWrite(fan, LOW); 
            Serial.println("fan deactivated");
            client.publish("teplica/fanOut", "0");
        }
    
    //Отопление
    } else if (strcmp(topic, "teplica/hot") == 0 && !hardOffModeActive) {
        if ((char)payload[0] == '1') { 
            digitalWrite(hot, HIGH); 
            Serial.println("hot activated");
            client.publish("teplica/hotOut", "1");
        } else if ((char)payload[0] == '0') {
            digitalWrite(hot, LOW); 
            Serial.println("hot deactivated");
            client.publish("teplica/hotOut", "0");
        }
    // Жесткое отключение. Если получаем teplica/hardoff то делаем аварийный стоп.
    } else if (strcmp(topic, "teplica/hardoff") == 0 && (char)payload[0] == '1') {
    Serial.println("Hard off mode activated");
    client.publish("teplica/hardoffOut", "1");
    hardOffModeActive = true; // Флаг hard off mode в истину
    digitalWrite(waterPump, LOW);
    digitalWrite(waterValve, LOW);
    digitalWrite(light, LOW); 
    digitalWrite(fan, LOW); 
    digitalWrite(hot, LOW);
    
    //Иначе держим hardOffModeActive аварийный стоп в сбросе
    } else if (strcmp(topic, "teplica/hardoff") == 0 && (char)payload[0] == '0') {
    Serial.println("Hard off mode deactivated");
    client.publish("teplica/hardoffOut", "0");
    hardOffModeActive = false; // Флаг hard off сброс
  
  //Перезагрузка контроллера. удаленная
  } else if (strcmp(topic, "teplica/reboot") == 0 && (char)payload[0] == '1') {
    client.publish("teplica/log", "reboot"); 
    Serial.println("rebooting");
    ESP.restart(); // Restart the controller
  
  //Если не один из режимов не отработал или флаг в трюе все отключаем
  } else {
    Serial.println("No action");
    // Reset pins to default state
    digitalWrite(waterPump, LOW);
    digitalWrite(waterValve, LOW);
    digitalWrite(light, LOW); 
    digitalWrite(fan, LOW); 
    digitalWrite(hot, LOW);
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

        ///////////////// Подписываемся на топик/////////////////
        // Полив
        client.subscribe("teplica/waterPump");
        //Набор воды
        client.subscribe("teplica/waterValve");
        //Освещение
        client.subscribe("teplica/light");
        //Вентиляция
        client.subscribe("teplica/fan");
        //Отопление
        client.subscribe("teplica/hot");
        //Аварийный стоп
        client.subscribe("teplica/hardoff");
        //Перезагрузка контроллера
        client.subscribe("teplica/reboot");
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
  pinMode(waterPump, OUTPUT); 
  pinMode(waterValve, OUTPUT); 
  pinMode(light, OUTPUT); 
  pinMode(fan, OUTPUT); 
  pinMode(hot, OUTPUT); 
  digitalWrite(waterPump, LOW);
  digitalWrite(waterValve, LOW);
  digitalWrite(light, LOW); 
  digitalWrite(fan, LOW); 
  digitalWrite(hot, LOW);
  
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
  
    // Проверка на необходимость перезагрузки каждые 4 часа
    if (millis() - lastRestartTime >= RESTART_INTERVAL) {
    Serial.println("Автоматическая перезагрузка через 4 часа");
    client.publish("teplica/log", "Auto reboot after 4 hours");
    ESP.restart(); // Перезагрузка контроллера
  }

  //Публикуем данные датчиков
  if (millis() % 5000 == 0) { // Проверяем каждые 5 секунд
    Serial.println(millis());
    Serial.print("Read sensors ");
    //int waterSensor = DigitalRead()
    //int waterSensor = DigitalRead()
    //int waterSensor = DigitalRead()
    client.publish("teplica/waterSensor", "25"); // Датчик уровня воды
    client.publish("teplica/TempSensor", "45"); // Датчик температуры
    client.publish("teplica/MoisterSensor", "90"); // Датчик влажности
  }

}

//Для удаленной прошивки
/* Выводить надпись, если такой страницы ненайдено */
void handleNotFound() {
  HttpServer.send(404, "text/plain", "404: Not found");
}

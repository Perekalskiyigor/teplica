#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

//прошивка удаленно
#define OTAUSER         "home"    // Логин для входа в OTA
#define OTAPASSWORD     "home"    // Пароль для входа в ОТА
#define OTAPATH         "/firmware"// Путь, который будем дописывать после ip адреса в браузере.
#define SERVERPORT      80         // Порт для входа, он стандартный 80 это порт http
ESP8266WebServer HttpServer(SERVERPORT);
ESP8266HTTPUpdateServer httpUpdater;



// Определение пинов
#define ledPin D1

int brightness = 0;       // Текущая яркость (0 - 255)
int fadeAmount = 5;       // Шаг изменения яркости (плавность)
unsigned long previousMillis = 0;  // Время последнего обновления яркости
unsigned long interval = 20;        // Интервал обновления яркости (20 мс)

#define Buzzer_PIN D4
#define Relay_PIN D5


// Глобальная переменная для отслеживания состояния режима hardoff
bool hardOffModeActive = false; 

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

//Функция обработки топиков
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Получено сообщение [");
  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  
    // Проверяем, что сообщение пришло от нужного нам топика
  if (strcmp(topic, "teplica/modeInput")) { 
    // Проверяем, не активен ли режим hardoff. Важно если активен 
    //все остальное рубится и уходит в No action

    //Mode 0 - режим все хорошо
      if ((char)payload[0] == '0') {
        unsigned long currentMillis = millis();  // Получаем текущее время
        Serial.println("Mode 0 input received");
        client.publish("teplica/modeOutput", "0"); // Публикуем ответ в топик bath/mode0Output
        // Проверяем, прошло ли нужное время для изменения яркости
        
        
        
        
        /*
        if (currentMillis - previousMillis >= interval) {
          // Сохраняем текущее время
          previousMillis = currentMillis;
      
          // Увеличиваем яркость
          brightness += fadeAmount;
      
          // Если яркость достигла максимума (255), то больше не увеличиваем
          if (brightness >= 255) {
            brightness = 255;  // Устанавливаем яркость на максимум
          }
      
          // Устанавливаем яркость светодиода с использованием ШИМ
          analogWrite(ledPin, brightness);
        }
        */
    }
      
    //Mode 1
     else if ((char)payload[0] == '1') {
      unsigned long currentMillis = millis();  // Получаем текущее время
      Serial.println("Mode 1 input received");
      client.publish("teplica/modeOutput", "1"); // Публикуем ответ в топик bath/mode0Output


      
      /*
      // Проверяем, прошло ли нужное время для изменения яркости
      if (currentMillis - previousMillis >= interval) {
        // Сохраняем текущее время
        previousMillis = currentMillis;
    
        // Уменьшаем яркость
        brightness -= fadeAmount;
    
        // Если яркость достигла минимума (0), то больше не уменьшаем
        if (brightness <= 0) {
          brightness = 0;  // Устанавливаем яркость на минимум
        }
    
        // Устанавливаем яркость светодиода с использованием ШИМ
        analogWrite(ledPin, brightness);
  }
  */
    } 
    
    //Mode 2
    else if ((char)payload[0] == '2') {
      digitalWrite(Buzzer_PIN, LOW); // Зуммер тихо
      Serial.println("Mode 2 input received");
      client.publish("bath/modeOutput", "2"); // Публикуем ответ в топик bath/mode0Output
    } 
    //Mode 3
   else if ((char)payload[0] == '3') {
        digitalWrite(Buzzer_PIN, HIGH); // Зуммер тихо
        Serial.println("Mode 3 input received");
        client.publish("bath/modeOutput", "3"); // Публикeуем ответ в топик bath/mode0Output
      }

    //Mode 4
    else if ((char)payload[0] == '4') {
        digitalWrite(Buzzer_PIN, HIGH); // Зуммер тихо
        Serial.println("Mode 4 input received");
        client.publish("bath/modeOutput", "4"); // Публикуем ответ в топик bath/mode0Output
      }

    // Жесткое отключение зумера без учета режимов
    } else if (strcmp(topic, "bath/hardoff") == 0 && (char)payload[0] == '1') {
    digitalWrite(Buzzer_PIN, HIGH); // Buzzer тихо
    Serial.println("Hard off mode activated");
    client.publish("bath/hardoffOut", "1");
    hardOffModeActive = true; // Флаг hard off mode в истину
    } else if (strcmp(topic, "bath/hardoff") == 0 && (char)payload[0] == '0') {
    // Disable all Modes and set pins to desired values
    digitalWrite(Buzzer_PIN, HIGH); // Buzzer тихо
    Serial.println("Hard off mode deactivated");
    client.publish("bath/hardoffOut", "0");
    hardOffModeActive = false; // Флаг hard off сброс
  
  //Mode 5
  } else if ((char)payload[0] == '5') {
    client.publish("bath/log", "reboot"); 
    Serial.println("rebooting");
    ESP.restart(); // Restart the controller
  
  //Если не один из режимов не отработал или флаг в трюе
  } else {
    Serial.println("No action");
    // Reset pins to default state
    digitalWrite(Buzzer_PIN, HIGH);
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
        client.subscribe("bath/modeInput");
        client.subscribe("bath/hardoff"); // Подписываемся на топик
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

  pinMode(ledPin, OUTPUT);  // Устанавливаем пин как выход


  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

   // Настройка Watchdog Timer на 30 секунд
  ESP.wdtEnable(30000);
  
  // Проверяем, не было ли перезапуска из-за зависания
  if (ESP.getResetReason() == "Software Watchdog") {
    Serial.println("Перезапуск из-за зависания!");
    client.publish("bath/log", "Watchdog timer activate");
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
    client.publish("bath/signal", String(signalLevel).c_str()); // Публикуем уровень сигнала
    
    // Преобразование IP-адреса в строку
    IPAddress ip = WiFi.localIP();
    String ipAddress = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);

    // Публикация IP-адреса в MQTT-топике
    client.publish("bath/ipAddress", ipAddress.c_str());

    client.publish("bath/Version", "2.3");
  }


  // Каждые 5 секунд и публикуем состояние реле
  if (millis() % 5000 == 0) { // Проверяем каждые 10 секун
    int relayLevel = digitalRead(Relay_PIN);
    Serial.print("relay: ");
    Serial.println(relayLevel);
    client.publish("bath/Relay", String(relayLevel).c_str()); // Публикуем состояние реле
  }
  
    // Проверка на необходимость перезагрузки каждые 4 часа
    if (millis() - lastRestartTime >= RESTART_INTERVAL) {
    Serial.println("Автоматическая перезагрузка через 4 часа");
    client.publish("bath/log", "Auto reboot after 4 hours");
    ESP.restart(); // Перезагрузка контроллера
  }
}

//Для удаленной прошивки
/* Выводить надпись, если такой страницы ненайдено */
void handleNotFound() {
  HttpServer.send(404, "text/plain", "404: Not found");
}

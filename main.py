import time
import paho.mqtt.client as mqtt
import datetime


# Полив
now = datetime.datetime.now()
months_range = range(1, 10)  # Месяцы с мая (5) по сентябрь (9)
start_hour = 5  # Начало временного интервала (5:00)
end_hour = 21    # Конец временного интервала (до 6:00)
hardwater =0 # принудитеьный полив
# Установите автополив на 5:00–6:00 (утро) и, при необходимости, на 18:00–19:00 (вечер) датчик поставим или по погоде ориентируемся.

# Освещение
months_range2 = range(1, 10)  # Месяцы с мая (5) по сентябрь (9)
start_hour2 = 5  # Начало временного интервала (5:00)
end_hour2 = 18    # Конец временного интервала (до 18:00)
day_status = "пасмурно"
hardlight =0 # принудитеьный свет
# Требуют 12–16 часов света в сутки 
# Май: дни еще короткие, возможны заморозки и пасмурная погода.
# Июнь–август: световой день длинный, но в пасмурные дни или при выращивании в теплице может потребоваться досветка.
# Сентябрь: дни становятся короче, освещение необходимо для продления вегетационного периода.

# Вентиляция
months_range3 = range(1, 10)  # Месяцы с мая (5) по сентябрь (9)
start_hour3 = 5  # Начало временного интервала (5:00)
end_hour3 = 21    # Конец временного интервала (до 6:00)
hardfan =0 # принудитеьный свет
maxTemp=0
tempteplica =0
# Открывать форточки:
# Для помидоров: при температуре выше 25°C.
# Для огурцов: при температуре выше 28°C.
# Закрывать форточки:
# Для помидоров: при температуре ниже 20°C.
# Для огурцов: при температуре ниже 22°C.

# Отопление
months_range4 = range(1, 10)  # Месяцы с мая (5) по сентябрь (9)
start_hour4 = 5  # Начало временного интервала (5:00)
end_hour4 = 21    # Конец временного интервала (до 6:00)
hardhot =0 # принудитеьный свет
minTemp=0
# Помидоры предпочитают температуру 20–25°C днем и 16–18°C ночью.


# Домашняя автоматизация
# Свет в ванне
start_hour5 = 5  # Начало временного интервала (5:00)
end_hour5 = 8    # Конец временного интервала (до 6:00)

start_hour6 = 18  # Начало временного интервала (5:00)
end_hour6 = 0    # Конец временного интервала (до 6:00)

hardlightBath =0 # принудитеьный свет


# MQTT настройки
BROKER = "37.79.202.158"
PORT = 1883
TOPIC_PUBLISH = "teplica/out/mode"
TOPIC_PUBLISH1 = "teplica/out/status"
TOPIC_SUBSCRIBE = "teplica/in/relayPump"
TOPIC_SUBSCRIBE1 = "teplica/in/inputSensor"
TOPIC_SUBSCRIBE2 = "teplica/in/TemmpSensor"
water =0


# Функция обратного вызова для обработки сообщений
def on_message(client, userdata, msg):
    global water
    payload = msg.payload.decode("utf-8")
    if msg.topic == TOPIC_SUBSCRIBE:
        if payload == "1":
            print("Включен насос")
    if msg.topic == TOPIC_SUBSCRIBE1:
        if payload == "1":
            water = 1
    if msg.topic == TOPIC_SUBSCRIBE2:
        if payload == "1":
            water = 1
            
        else:
            water = 0


# Создание MQTT клиента
client = mqtt.Client()

# Установка функций обратного вызова
client.on_message = on_message

try:
    # Подключение к брокеру
    client.connect(BROKER, PORT, keepalive=60)

    # Подписка на топики
    client.subscribe(TOPIC_SUBSCRIBE)
    client.subscribe(TOPIC_SUBSCRIBE1)

    # Запуск цикла обработки сообщений в отдельном потоке
    client.loop_start()

    while True:
        # Публикация сообщения в топик
        client.publish(TOPIC_PUBLISH1, "ok")
        print(f"Опубликовано в {TOPIC_PUBLISH1}: ok")

        # Задержка на 5 секунд
        time.sleep(5)
        named_tuple = time.localtime() # получить struct_time
        time_string = time.strftime("%m/%H:%M")

        #Полив
        # Проверяем, что месяц в заданном интервале и время в пределах 5:00 - 6:00
        if now.month in months_range and start_hour <= now.hour < end_hour or hardwater ==1:
            client.publish('teplica/waterPump', "1")
            print("Полив включено.")
        else:
            client.publish('teplica/waterPump', "6")

        #Наолнение воды
        if now.month in months_range and start_hour <= now.hour < end_hour and water ==0:
            client.publish('teplica/waterValve', '3')
            print("Набор воды в бочку включен.")
        else:
            client.publish('teplica/waterValve', '7')

        #Освещение
        # Проверяем, что месяц в заданном интервале и время в пределах 7:00 - 21:00 смртрим по апи прогоноз если пасмурно
        if now.month in months_range2 and start_hour2 <= now.hour < end_hour2 and day_status == 'пасмурно' or hardlight ==1:
            client.publish('teplica/light', "2")
            print("Освещение включено.")
        else:
            client.publish('teplica/light', "7")

        #Вентиляция
        # Проверяем, что месяц в заданном интервале и время в пределах 7:00 - 21:00 смртрим температуру
        if now.month in months_range3 and start_hour3 <= now.hour < end_hour3 and tempteplica > maxTemp or hardfan ==1:
            client.publish('teplica/fan', "2")
            print("Вентиляция включено.")
        else:
            client.publish('teplica/fan', "7")
        
        #Отопление
        # Проверяем, что месяц в заданном интервале и время в пределах 7:00 - 21:00 смртрим по апи температуру
        if now.month in months_range4 and start_hour4 <= now.hour < end_hour4 and tempteplica >= minTemp or hardhot ==1:
            client.publish('teplica/hot', "2")
            print("Отопление включено.")
        else:
            client.publish('teplica/hot', "7")


        # Домашняя автоматизация #Отопление
        # Внна включаем свет по расписанию
        if start_hour5 <= now.hour < end_hour5 or hardhot ==1:
            client.publish('home/bath', "1")
            print("Свет в ванне включен")
        else:
            client.publish('home/bath', "2")

        
        
 
        print(time_string)
except KeyboardInterrupt:
    print("Отключено.")
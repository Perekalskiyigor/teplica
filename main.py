import time
import paho.mqtt.client as mqtt
import datetime

# Импортируем функцию из модуля weather
from weather import get_weather

# Полив
months_range = range(2, 2)  # Месяцы с мая (5) по сентябрь (9)
start_hour = 15  # Начало временного интервала (5:00)
end_hour = 17    # Конец временного интервала (до 6:00)
hardwater = 0  # принудительный полив

# Набор воды
months_range7 = range(2, 2)  # Месяцы с мая (5) по сентябрь (9)
start_hour7 = 15  # Начало временного интервала (5:00)
end_hour7 = 17    # Конец временного интервала (до 6:00)
hardwater7 = 0  # принудительный полив

# Освещение

"""
Если облачность небольшая (например, 0-50%):
Период включения освещения: с 9:00 до 16:00.
Использовать мощность ламп в пределах средней интенсивности, например, 300 мкмоль/м²/с для помидоров и дынь.
Если облачность умеренная или высокая (более 50%):
Период включения освещения: с 8:00 до 18:00.
Увеличить мощность освещения, например, до 500 мкмоль/м²/с для помидоров и дынь.
"""
months_range2 = range(2, 11)  # все лето
start_hour2 = 8  # Начало временного интервала (8:00)
end_hour2 = 18    # Конец временного интервала (до 18:00)
day_status = ""
hardlight = 0  # принудительный свет

# Свет на весенний период
months_range8 = range(2, 5)  # Весной (март — апрель)
start_hour8 = 8  # Начало временного интервала (8:00)
end_hour8 = 18    # Конец временного интервала (до 18:00)

####### Вентиляция
months_range3 = range(2, 4)  # Месяцы с мая (5) по сентябрь (9)
start_hour3 = 5  # Начало временного интервала (5:00)
end_hour3 = 21    # Конец временного интервала (до 6:00)
hardfan = 0  # принудительный свет
maxTemp = 0
temperature = 0

# Отопление
months_range4 = range(1, 2)  # Месяцы с мая (5) по сентябрь (9)
start_hour4 = 5  # Начало временного интервала (5:00)
end_hour4 = 21    # Конец временного интервала (до 6:00)
hardhot = 0  # принудительный свет
minTemp = 0

# Домашняя автоматизация
start_hour5 = 5  # Начало временного интервала (5:00)
end_hour5 = 8    # Конец временного интервала (до 6:00)

start_hour6 = 18  # Начало временного интервала (5:00)
end_hour6 = 0    # Конец временного интервала (до 6:00)

hardlightBath = 0  # принудительный свет

# Баня
hardBana = 1  # добавляем переменную для принудительного включения бака
pump_start_time = None  # для отслеживания времени начала работы насоса
pump_duration = 2 * 60  # продолжительность работы насоса в секундах (50 минут)
is_pump_active = True  # флаг для отслеживания состояния насоса

# MQTT настройки
BROKER = "37.79.202.158"
PORT = 1883
TOPIC_PUBLISH1 = "teplica/out/status"
waterSensor = "teplica/in/waterSensor"
TempSensor = "teplica/in/TempSensor"
MoisterSensor = "teplica/in/TemmpSensorMoisterSensor"

# Создание MQTT клиента
client = mqtt.Client()

# Инициализация переменных
water = 1 # 0 нет воды в бочке
temperature = 0
moister = 0

# Функция обработки полученных сообщений
def on_message(client, userdata, message):
    global water
    global temperature
    global moister
    print(f"****{message.topic}")

    if message.topic == waterSensor:
        # Получаем данные с датчика уровня воды
        water = int(message.payload.decode())
        print(f"Уровень воды: {water}")

    elif message.topic == TempSensor:
        # Получаем температуру с датчика температуры
        temperature = float(message.payload.decode())
        print(f"Температура в теплице: {temperature}")

    elif message.topic == MoisterSensor:
        # Получаем данные с датчика влажности
        moister = float(message.payload.decode())
        print(f"Влажность почвы: {moister}")


# Устанавливаем функцию обработки сообщений
client.on_message = on_message


# Подключение и подписка на топики
def connect_and_subscribe():
    client.connect(BROKER, PORT, keepalive=60)
    client.subscribe(waterSensor)
    client.subscribe(TempSensor)
    client.subscribe(MoisterSensor)



# Подключаемся и подписываемся на топики
connect_and_subscribe()

# Запуск цикла обработки сообщений в отдельном потоке
client.loop_start()



# Ваш API ключ
api_key = "c491a9ce74952b873e20b5bf2d8a5a7e"

# Координаты для Екатеринбурга, Белоярский район, поселок Прохладный
latitude = 56.8519   # Широта
longitude = 60.6367  # Долгота


try:
    while True:
        # Обновление времени
        now = datetime.datetime.now()

        # Получение данных о погоде
        weather_info = get_weather(api_key, latitude, longitude)

        # Вывод информации о погоде
        if isinstance(weather_info, dict):
            print(f"Погода в {weather_info['city']}:")
            print(f"Описание: {weather_info['description']}")
            day_status = weather_info['description']
            print(f"Температура: {weather_info['temperature']}°C")
            print(f"Влажность: {weather_info['humidity']}%")
            print(f"Скорость ветра: {weather_info['wind_speed']} м/с")
        else:
            print(weather_info)


        # Публикация сообщения в топик
        client.publish(TOPIC_PUBLISH1, "ok")
        print(f"Опубликовано в {TOPIC_PUBLISH1}: ok")

        # Задержка на 5 секунд
        time.sleep(5)
        named_tuple = time.localtime()  # получить struct_time
        time_string = time.strftime("%m/%H:%M")

        ########################## Полив
        # Проверяем, что месяц в заданном интервале и время в пределах 15:00 - 17:00
        if now.month in months_range and start_hour <= now.hour < end_hour or hardwater == 1:
            client.publish('teplica/waterPump', "1")
            print("Полив включен.")
        else:
            client.publish('teplica/waterPump', "0")

        ########################## Набор воды
        if now.month in months_range7 and start_hour7 <= now.hour < end_hour7 and water == 0:
            client.publish('teplica/waterValve', '1')
            print("Набор воды в бочку включен.")
        else:
            client.publish('teplica/waterValve', '0')

        ########################### Освещение
        # (Автомат по погоде) Если облачность небольшая (например, 0-50%): Период включения освещения: с 9:00 до 18:00
        if now.month in months_range2 and start_hour2 <= now.hour < end_hour2 and day_status == 'небольшая облачность' or hardlight == 1:
            client.publish('teplica/light', "1")
            print("Освещение включено.")
        else:
            client.publish('teplica/light', "0")

        # Весной (март — апрель):
        # Рекомендуемое время для досвечивания: с 8:00 до 18:00  в пасмурные дни или в случае, если освещенность ниже досвечиваем.
        if now.month in months_range8 and start_hour8 <= now.hour < end_hour8 and day_status in 'облачность' or hardlight == 1:
            client.publish('teplica/light', "1")
            print("Освещение включено.")
        else:
            client.publish('teplica/light', "0")

        # Вентиляция
        if now.month in months_range3 and start_hour3 <= now.hour < end_hour3 and temperature > maxTemp or hardfan == 1:
            client.publish('teplica/fan', "1")
            print("Вентиляция включена.")
        else:
            client.publish('teplica/fan', "0")

        # Отопление
        if now.month in months_range4 and start_hour4 <= now.hour < end_hour4 and temperature <= minTemp or hardhot == 1:
            client.publish('teplica/hot', "1")
            print("Отопление включено.")
        else:
            client.publish('teplica/hot', "0")

        
        """
                if hardBana == 1:
            if is_pump_active:
                if pump_start_time is None:
                    pump_start_time = time.time()  # Начинаем отсчет времени, только если насос включен
                print("Баня-помпа включена.")
                client.publish('bana/pump', "1")
            elif time.time() - pump_start_time >= pump_duration:
                # Ожидаем 2 минуты (время работы помпы) и выключаем насос
                print("Баня-помпа выключена.")
                client.publish('bana/pump', "0")
                is_pump_active = False  # Останавливаем насос и меняем состояние
                pump_start_time = None  # Обнуляем время начала работы насоса

            # Чтобы перезапустить насос после выключения, можно добавить следующую проверку:
            # Например, когда принудительно нужно включить насос снова, проверка может быть такая:
            if hardBana == 1 and not is_pump_active:
                # Перезапускаем насос
                is_pump_active = True
                pump_start_time = None  # Сброс времени начала работы
                print("Перезапуск насоса.")
                client.publish('bana/pump', "1")
        """
        

        
        
 
        print(time_string)
except KeyboardInterrupt:
    print("Отключено.")
import time
import paho.mqtt.client as mqtt
from datetime import datetime, timedelta
from zoneinfo import ZoneInfo
import logging

# Настройка логгера
logging.basicConfig(
    filename="teplica.log",
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s"
)

# Импортируем функцию из модуля weather
from weather import get_weather

# [(начало_дождя, конец_дождя), ...]
rain_periods = []
raining_now = False
rain_start = None

# Полив
months_range = range(5, 10)  # Месяцы с мая (5) по сентябрь (9)
start_hour = 6  # Начало временного интервала (6:00)
end_hour = 9    # Конец временного интервала (до 9:00)
hardwater = 0  # принудительный полив




# Набор воды
months_range7 = range(5, 10)  # Месяцы с мая (5) по сентябрь (9)
start_hour7 = 9  # Начало временного интервала (5:00)
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
months_range2 = range(5, 11)  # все лето
start_hour2 = 8  # Начало временного интервала (8:00)
end_hour2 = 18    # Конец временного интервала (до 18:00)
day_status = ""
hardlight = 0  # принудительный свет

# Свет на весенний период
months_range8 = range(5, 11)  # Весной (март — апрель)
start_hour8 = 8  # Начало временного интервала (8:00)
end_hour8 = 18    # Конец временного интервала (до 18:00)

####### Вентиляция
months_range3 = range(5, 10)  # Месяцы с мая (5) по сентябрь (9)
start_hour3 = 5  # Начало временного интервала (5:00)
end_hour3 = 21    # Конец временного интервала (до 6:00)
hardfan = 0  # принудительный свет
maxTemp = 30
temperature = 0

# Отопление
months_range4 = range(5, 10)  # Месяцы с мая (5) по сентябрь (9)
start_hour4 = 1  # Начало временного интервала (5:00)
end_hour4 = 23    # Конец временного интервала (до 6:00)
minTemp = 16


# Полив на улице
months_range9 = range(5, 10)  # Месяцы с мая (5) по сентябрь (9)
start_hour9 = 4  # Начало временного интервала (5:00)
end_hour9 = 15    # Конец временного интервала (до 6:00)



# Домашняя автоматизация
start_hour5 = 5  # Начало временного интервала (5:00)
end_hour5 = 8    # Конец временного интервала (до 6:00)

start_hour6 = 18  # Начало временного интервала (5:00)
end_hour6 = 0    # Конец временного интервала (до 6:00)

hardlightBath = 0  # принудительный свет

# Баня
hardBana = 1  # добавляем переменную для принудительного включения бака
pump_start_time = None  # для отслеживания времени начала работы насоса
pump_duration = 10  # Насос работает 10 секунд
pump_interval = 5 * 60  # Насос включается каждые 5 минут
last_pump_time = 0  # Время последнего включения насоса
is_pump_active = False  # флаг для отслеживания состояния насоса

# MQTT настройки
BROKER = "37.79.202.158"
PORT = 1883
TOPIC_PUBLISH1 = "teplica/out/status"
waterSensor = "teplica/SensorWater"
TempSensor = "teplica/TempSensor"
MoisterSensor = "teplica/in/TemmpSensorMoisterSensor"

# Создание MQTT клиента
client = mqtt.Client()

# Инициализация переменных
water = 1 # 0 нет воды в бочке
temperature = 0
moister = 0

# Функция расчетв времени  дождя для полива
def total_rain_duration_in_hours(periods, now):
    total = 0
    cutoff = now - timedelta(days=1)  # только последние сутки
    for start, end in periods:
        # Игнорируем слишком старые
        if end < cutoff:
            continue
        # Ограничиваем по суткам
        start = max(start, cutoff)
        delta = end - start
        total += delta.total_seconds() / 3600
    return total

def on_message(client, userdata, message):
    global water, temperature, moister
    topic = message.topic
    payload = message.payload.decode()

    if topic == waterSensor:
        water = int(payload)
        # print(f"Уровень воды: {water}")
        logging.info(f"[MQTT] Уровень воды: {water}")

    elif topic == TempSensor:
        temperature = float(payload)
        # print(f"Температура в теплице: {temperature}")
        logging.info(f"[MQTT] Температура: {temperature}")

    elif topic == MoisterSensor:
        moister = float(payload)
        # print(f"Влажность почвы: {moister}")
        logging.info(f"[MQTT] Влажность почвы: {moister}")


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
        now = datetime.now(ZoneInfo("Asia/Yekaterinburg"))

        # #  Подставляем тестовые дожди
        # rain_periods = [
        #     (now - timedelta(hours=1), now - timedelta(minutes=30)),
        #     (now - timedelta(minutes=20), now - timedelta(minutes=5)),
        #     (now - timedelta(hours=26), now - timedelta(hours=25)),
        # ]

        print(f"Work OK {now}")

        # Получение данных о погоде
        weather_info = get_weather(api_key, latitude, longitude)

        # Вывод информации о погоде
        if isinstance(weather_info, dict):
            print(f"Погода в {weather_info['city']}:")
            print(f"Описание: {weather_info['description']}")
            day_status = weather_info['description']
            print(f"Температура: {weather_info['temperature']}°C")
            temperature = weather_info['temperature']
            print(f"Влажность: {weather_info['humidity']}%")
            print(f"Скорость ветра: {weather_info['wind_speed']} м/с")

            logging.info(f"Погода: {day_status}, Темп: {temperature}°C, Влажн: {weather_info['humidity']}%, Ветер: {weather_info['wind_speed']} м/с")
        else:
            print(weather_info)
            logging.warning(f"Ошибка погоды: {weather_info}")

        client.publish(TOPIC_PUBLISH1, "ok")
        print(f"Опубликовано в {TOPIC_PUBLISH1}: ok")
        logging.info(f"Публикация: {TOPIC_PUBLISH1} → ok")


        
        named_tuple = time.localtime()  # получить struct_time
        time_string = time.strftime("%m/%H:%M")

        # Остлеживание дождя
        desc = weather_info['description'].lower()
        rain_detected = "дожд" in desc  # покрывает "дождь", "небольшой дождь", "дождливо"

        # Начало дождя
        if rain_detected and not raining_now:
            rain_start = now
            raining_now = True
            print(f"[LOG] Начался дождь в {rain_start}")

        # Конец дождя
        elif not rain_detected and raining_now:
            rain_periods.append((rain_start, now))
            raining_now = False
            print(f"[LOG] Дождь закончился в {now}. Текущие периоды дождя: {rain_periods}")
            print(f"[LOG] Всего периодов дождя (включая текущий): {len(temp_rain_periods)}")
            print(f"[LOG] Периоды дождя: {temp_rain_periods}")

        # Подсчёт с учётом текущего дождя
        temp_rain_periods = list(rain_periods)
        if raining_now:
            temp_rain_periods.append((rain_start, now))

        # Задержка на 5 секунд
        time.sleep(5)

        ########################## Полив на улице
        # Проверяем, что месяц в заданном интервале и время в пределах 05:00 - 07:00 и четный

        total_hours = total_rain_duration_in_hours(temp_rain_periods, now)

        if (now.month in months_range9 and start_hour9 <= now.hour < end_hour9 and
            now.day % 2 == 0 and total_hours < 2): 
            client.publish('waterSystem/Valve1/in', "1")
            print("[Режим] Полив на улице включен.")
            logging.info("Полив на улице включен.")
            rain_periods.clear()
            raining_now = False
            rain_start = None
        else:
            client.publish('waterSystem/Valve1/in', "0")
            # print("Полив на улице выключен.")
            # logging.info("Полив на улице выключен.")

        ########################## Полив
        # Проверяем, что месяц в заданном интервале и время в пределах 15:00 - 17:00
        if now.month in months_range and start_hour <= now.hour < end_hour:
            client.publish('teplica/waterPump/in', "1")
            print("[Режим] Полив включен.")
            logging.info("Полив включен.")
        else:
            client.publish('teplica/waterPump/in', "0")
            # print("Полив выключен.")
            # logging.info("Полив выключен.")

        ########################## Набор воды
        if now.month in months_range7 and start_hour7 <= now.hour < end_hour7 and water == 0:
            client.publish('teplica/waterValve/in', '1')
            print("[Режим] Набор воды в бочку включен.")
            logging.info("Набор воды в бочку включен.")
        else:
            client.publish('teplica/waterValve/in', '0')
            # print("Набор воды в бочку выключен.")
            # logging.info("Набор воды в бочку выключен.")

        ########################### Освещение
        # Объединенное условие освещения для весны и лета
        if (
            ("облачно" in day_status.lower() or "дожд" in day_status.lower() or "пасмурно" in day_status.lower()) and
            (
                (now.month in months_range2 and start_hour2 <= now.hour < end_hour2) or
                (now.month in months_range8 and start_hour8 <= now.hour < end_hour8)
            )
        ):
            client.publish('teplica/light/in', "1")
            print("[Режим] Освещение включено.")
            logging.info(f"Освещение включено — статус дня: {day_status}, время: {now.hour}")
        else:
            client.publish('teplica/light/in', "0")
            # print("Освещение выключено.")
            # logging.info(f"Освещение выключено — статус дня: {day_status}, время: {now.hour}")

        # Вентиляция
        if now.month in months_range3 and start_hour3 <= now.hour < end_hour3 and temperature > maxTemp:
            client.publish('teplica/fan/in', "1")
            print("[Режим] Вентиляция включена.")
            logging.info(f"Вентиляция включена — температура: {temperature}°C, порог: {maxTemp}°C")
        else:
            client.publish('teplica/fan/in', "0")
            #print("Вентиляция выключена.")
            #logging.info(f"Вентиляция выключена — температура: {temperature}°C, порог: {maxTemp}°C")

        # Отопление
        if now.month in months_range4 and start_hour4 <= now.hour < end_hour4 and temperature <= minTemp:
            client.publish('teplica/hot/in', "1")
            print("[Режим] Отопление включено.")
            logging.info(f"Отопление включено — температура: {temperature}°C, порог: {minTemp}°C")
        else:
            client.publish('teplica/hot/in', "0")
            #print("Отопление выключено.")
            #logging.info(f"Отопление выключено — температура: {temperature}°C, порог: {minTemp}°C")

        
        # Баня управление паром

        if hardBana == 1:
            current_time = time.time()
            # logging.debug(f"Pump logic: now={now}, current_time={current_time}, last_pump_time={last_pump_time}, active={is_pump_active}")
            # Проверка: прошло ли 5 минут с последнего запуска
            if current_time - last_pump_time >= pump_interval and not is_pump_active:
                is_pump_active = True
                pump_start_time = current_time
                last_pump_time = current_time
                # logging.info("Насос включён.")
                client.publish('bana/pump', "1")

            # Проверка: прошло ли 10 секунд с начала работы
            if is_pump_active and (current_time - pump_start_time >= pump_duration):
                is_pump_active = False
                pump_start_time = None
                # logging.info("Насос выключен.")
                client.publish('bana/pump', "0")

        else:
            # Если hardBana выключен, убедимся, что насос не работает
            if is_pump_active:
                is_pump_active = False
                pump_start_time = None
                #logging.warning("Принудительное выключение насоса: hardBana отключен.")
                client.publish('bana/pump', "0")
        

        
        
 
        print(time_string)
except KeyboardInterrupt:
    print("Отключено.")



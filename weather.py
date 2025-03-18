import requests

def get_weather(api_key, latitude, longitude):
    """
    Функция для получения погоды по координатам.

    :param api_key: Ваш API ключ для OpenWeatherMap.
    :param latitude: Широта.
    :param longitude: Долгота.
    :return: Словарь с информацией о погоде или сообщение об ошибке.
    """
    # Формирование URL для запроса
    url = f"https://api.openweathermap.org/data/2.5/weather?lat={latitude}&lon={longitude}&appid={api_key}&units=metric&lang=ru"
    
    try:
        # Выполнение запроса
        response = requests.get(url)
        
        # Проверка на успешный ответ
        if response.status_code == 200:
            data = response.json()
            
            # Извлечение информации о погоде
            city_name = data["name"]
            weather_description = data["weather"][0]["description"]
            temperature = data["main"]["temp"]
            humidity = data["main"]["humidity"]
            wind_speed = data["wind"]["speed"]
            
            # Формирование результата
            weather_info = {
                "city": city_name,
                "description": weather_description,
                "temperature": temperature,
                "humidity": humidity,
                "wind_speed": wind_speed
            }
            return weather_info
        else:
            return f"Ошибка запроса: {response.status_code}"
    except Exception as e:
        return f"Произошла ошибка: {str(e)}"



"""
# Ваш API ключ
api_key = "c491a9ce74952b873e20b5bf2d8a5a7e"

# Координаты для Екатеринбурга, Белоярский район, поселок Прохладный
latitude = 56.8519   # Широта
longitude = 60.6367  # Долгота

# Получение данных о погоде
weather_info = get_weather(api_key, latitude, longitude)

# Вывод информации о погоде
if isinstance(weather_info, dict):
    print(f"Погода в {weather_info['city']}:")
    print(f"Описание: {weather_info['description']}")
    print(f"Температура: {weather_info['temperature']}°C")
    print(f"Влажность: {weather_info['humidity']}%")
    print(f"Скорость ветра: {weather_info['wind_speed']} м/с")
else:
    print(weather_info)
"""
# Ваш API ключ
api_key = "c491a9ce74952b873e20b5bf2d8a5a7e"

# Координаты для Екатеринбурга, Белоярский район, поселок Прохладный
latitude = 56.8519   # Широта
longitude = 60.6367  # Долгота

# Получение данных о погоде
weather_info = get_weather(api_key, latitude, longitude)

# Вывод информации о погоде
if isinstance(weather_info, dict):
    print(f"Погода в {weather_info['city']}:")
    print(f"Описание: {weather_info['description']}")
    print(f"Температура: {weather_info['temperature']}°C")
    print(f"Влажность: {weather_info['humidity']}%")
    print(f"Скорость ветра: {weather_info['wind_speed']} м/с")
else:
    print(weather_info)
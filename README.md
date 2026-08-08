# TARS — Monitor de Calidad del Aire ESP32

Sistema embebido basado en ESP32 para monitoreo de partículas suspendidas (PM1.0, PM2.5, PM10), con máquina de estados modular, pantalla OLED interactiva y envío de datos a un broker FIWARE Orion.

![Plataforma](https://img.shields.io/badge/plataforma-ESP32-blue)
![Lenguaje](https://img.shields.io/badge/lenguaje-C++-orange)
![Licencia](https://img.shields.io/badge/licencia-MIT-green)

## Características

- 📊 Lectura de PM1.0 / PM2.5 / PM10 vía sensor DFRobot SEN0460 (I2C)
- 🖥️ 5 pantallas interactivas en OLED (calidad de aire, categoría EPA, estadísticas, tendencia, red WiFi)
- 🔘 Navegación con botón físico (pulsación corta/larga)
- 📡 Envío periódico a FIWARE Orion Context Broker con autenticación OAuth2 (Keyrock)
- ⚙️ Modo desarrollador con portal web para configuración OTA (WiFi, intervalos, servidor)
- 🏗️ Arquitectura modular: máquina de estados + separación de responsabilidades (sensores, pantalla, red, payload)

## Arquitectura

```
StateMachine
├── EstadoINICIO       → conexión WiFi, primer boot
├── EstadoLECTURA       → lectura periódica del sensor + refresco de pantalla
├── EstadoENVIO         → PATCH a Orion / POST a agente Flask
└── EstadoDESARROLLADOR → portal web de configuración (AP o STA)
```

## Estructura del proyecto

```
AIRQ/
├── AIRQ.ino              → setup(), loop(), objetos globales
├── AppConfig.h            → configuración persistente (NVS)
├── ButtonHandler.h        → detección de pulsación corta/larga
├── DevWebOTA.h/.cpp       → portal web de configuración + OTA
├── DisplayManager.h/.cpp  → renderizado de las 5 pantallas OLED
├── Estados.h/.cpp         → lógica de cada estado (INICIO, LECTURA, ENVIO, DESARROLLADOR)
├── PayloadBuilder.h/.cpp  → construcción del JSON para Orion
├── SensorManager.h/.cpp   → lectura y promediado del sensor de aire
├── State.h                → interfaz base de estado
├── StateMachine.h/.cpp    → máquina de estados genérica
├── TokenManager.h         → autenticación OAuth2 con Keyrock
└── WiFiManager.h          → conexión/reconexión WiFi
```

## Hardware

| Componente | Conexión |
|---|---|
| ESP32 DevKit | — |
| Pantalla OLED SSD1306 128x64 (I2C) | SDA→21, SCL→22, VCC→5V, GND→GND |
| Sensor DFRobot SEN0460 (I2C, 0x19) | SDA→21, SCL→22 |
| Botón pulsador | GPIO 4 → GND (pull-up interno) |

## Instalación

1. Instala las librerías desde el Gestor de Librerías del IDE de Arduino:
   - `Adafruit GFX Library`
   - `Adafruit SSD1306`
   - `DFRobot_AirQualitySensor`
   - `ArduinoJson`
2. Abre `AIRQ/AIRQ.ino`
3. Selecciona la placa ESP32 correspondiente en `Herramientas > Placa`
4. Sube el código
5. En el primer arranque, conéctate al AP `TARS-tars-new` (contraseña `12345678`) y configura tu red WiFi desde `192.168.4.1`

## Pantallas disponibles

| # | Pantalla | Contenido |
|---|---|---|
| 0 | Calidad de aire | Barras de PM1.0/PM2.5/PM10 |
| 1 | PM2.5 destacado | Valor grande + categoría EPA |
| 2 | Estadísticas | Min/Max/Muestras del intervalo actual |
| 3 | Tendencia | Sparkline de últimas 20 lecturas |
| 4 | Red WiFi | SSID, RSSI, hostname |

## Capturas

![Vista calidad de aire](docs/pantalla-calidad-aire.jpg)
![Portal de configuración](docs/portal-web.png)

## Créditos

Proyecto desarrollado por [Fer](https://github.com/1cfer) — Universidad Pontificia Bolivariana, Semillero AgeVital.

## Licencia

MIT — ver [LICENSE](LICENSE)

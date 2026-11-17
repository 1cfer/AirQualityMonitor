# Revisión air-2.0.0 — 17 de noviembre de 2026

La lectura permanece visible durante todo el envío. El trabajo de red se ejecuta en una tarea independiente: OAuth, un servidor lento o un error HTTP ya no detienen el botón ni el muestreo normal.

Base: `1cfer/AirQualityMonitor`, commit `ad2e966729484bbfcd78002a9edf2e34b3f3933e`. Rama local de trabajo: `codex/oled-background-telemetry`.

Referencia del modo desarrollador: copia local de TARS, rama `codex/tars-legacy-1.1.1`, commit verificado `cd431c317d2fc669f05a1d15c6a4492e21a24b9c`, y su `REVISION.md`. Esa copia de TARS no se modificó. Se adaptaron el botón, WiFi, máquina de estados, transporte, OAuth y portal a este monitor de partículas.

## Cambios

| Área | Resultado |
|---|---|
| Pantalla normal | Cinco páginas con geometría explícita de 128×64, cifras ampliadas, texto sin salto automático y nombres largos abreviados. PM2.5 es la vista principal; no se reemplaza con «ESTADO: ENVIO». |
| Indicadores | WiFi, TX, último resultado y página en el encabezado. Antigüedad y estado del sensor en las vistas de lectura. Cero y lectura ausente se distinguen. |
| Tendencia | Anillo de 60 lecturas válidas, escala calculada sobre exactamente los datos dibujados, también antes de llenar el anillo. Mínimo/máximo/media usan el mismo historial. |
| Datos | Las estadísticas no se reinician al transmitir. Se conservan los registros STANDARD y los nombres NGSIv2 `pm1_0`, `pm2_5`, `pm10`. No se etiqueta una lectura instantánea como categoría EPA/AQI. |
| Red | Una tarea FreeRTOS creada una sola vez y dos colas de capacidad 1. Sólo el ciclo principal accede a I2C, pantalla, configuración y acumuladores. La tarea recibe una instantánea de configuración y no escribe NVS. |
| Reintentos | El lote se separa de la acumulación nueva. Sólo se confirma con 2xx. En un fallo se combina con las muestras nuevas, sin borrarlas. Reintento contado desde la finalización; máximo una petición en curso. |
| Memoria | Sumas enteras acotadas con reducción proporcional de peso a partir de acumulaciones prolongadas. No se crean estados ni tareas para cada envío. Promedio sin muestras no se transmite como dato nuevo. |
| Sensor | Lectura I2C con comprobación de ACK y longitud. Una lectura incompleta no se contabiliza como cero ni se mezcla con canales anteriores. Recuperación automática al siguiente intervalo. Driver DFRobot externo innecesario. |
| Estados | Destructor virtual y transición diferida hasta terminar `execute()`, evitando destruir el estado que aún está ejecutándose. |
| Arranque/OLED | Arranque sin esperar conexión WiFi. Búsqueda OLED en 0x3C/0x3D; el equipo puede seguir sin pantalla. Timeout I2C, bus compartido conservado a 50 kHz, apagado real y despertar por botón. |
| Botón | Debounce, clic, doble clic y acción única a los 4 s. El primer clic con OLED apagada sólo la despierta. |
| WiFi | AP+STA, tres redes guardadas y migración de `ssid/pass` del firmware original. Escaneo asíncrono, tres filas, A/C/*, elección con 4 s, cancelación con doble clic, timeout de 12 s y recuperación de la red anterior. |
| Configuración | Claves NVS válidas `readMs`, `retryMs`, `screenMs`; límites de intervalos al cargar y guardar. Validación de hostname, URL, contraseña WiFi y tamaños de credenciales en el servidor. |
| HTTP/OAuth | Cliente correcto para http/https, tiempos de conexión y lectura, timeout TLS, sin redirecciones automáticas, codificación de formulario y expiración de token sin underflow/rollover. Sin imprimir tokens ni cuerpos de autenticación en Serial. |
| Modo desarrollador | Selección WiFi de TARS 1.1.1, portal con valores actuales, secretos sin rellenar, lecturas locales, identificación real de OLED, salida desde el panel, OTA y restauración. Continúa midiendo durante configuración y pausa la transmisión. |
| OTA | Fallo y aborto devuelven error sin reiniciar. Reinicio exitoso diferido, sin `delay()` en la ruta. Se impide salir del modo durante OTA o un reinicio programado. |
| AMERI | Sin integración ni verificaciones hacia AMERI. El único sondeo añadido es del navegador a `/status` del propio ESP32, cada 3 s mientras la pestaña es visible. El agente POST original permanece opcional y desactivado por defecto. |

## Verificación

Las pruebas nativas compilan los `.cpp` de producción, incluido el portal real, con ArduinoJson instalado y sustitutos de hardware/red. La tarea de red se ejecuta en un hilo separado del equipo de pruebas. La prueba retiene deliberadamente una petición HTTP mientras avanza lecturas, páginas y frames; después comprueba fallo, restauración del lote, éxito y conservación de las muestras nuevas.

También cubren lectura parcial/desconexión/recuperación I2C, historial circular, valores 0–65534, acumulaciones prolongadas y fusión de lotes impares, rollover, claves NVS con el límite de 15 caracteres, clic/doble clic/4 s, transición diferida, migración WiFi, red abierta y timeout, escape HTML, rechazo de configuración sin cambios parciales y OTA fallida/abortada/exitosa. El mock de OLED usa la fuente real de Adafruit GFX y rechaza texto fuera de pantalla o superpuesto.

Resultado nativo: correcto, sin advertencias de compilación. El JavaScript del panel pasa `node --check`. `git diff --check` correcto tras normalizar espacios finales.

Compilación final ESP32: correcta, salida 0 y sin advertencias. Flash 1186940 / 1310720 bytes; RAM estatica 53720 / 327680 bytes. Se cotejaron los 24 archivos del sketch con la copia compilada, ignorando solamente espacios finales, lineas vacias y directivas/prototipos generados por Arduino. Registro: `VALIDATION_ESP32.log`. Placa `esp32:esp32:esp32da`, núcleo 3.3.12, ArduinoJson 7.4.3, SSD1306 2.5.17, GFX 1.13.0 y BusIO 1.17.4. El binario corresponde a esa placa y partición; para otro modelo se debe compilar el sketch con su configuración.

Las vistas previas son renderizados del framebuffer de las pruebas, no fotografías de un dispositivo. Se entregan el sketch completo, pruebas y documentación. La revisión se publica en GitHub con el binario de aplicación para OTA en `firmware/`, instrucciones de carga y SHA-256.

## Límites prácticos

No se ha cargado ni probado este firmware en un ESP32 físico. Comprobar sensor SEN0460, OLED 128×64, botón GPIO4, desconexión/reconexión WiFi, destino real y actualización OTA en hardware. La prueba con hilo nativo no sustituye la validación del planificador FreeRTOS y del consumo de pila en el ESP32.

Los lotes pendientes están en RAM; se pierden al reiniciar. La reducción de peso conserva un promedio acotado, no un archivo histórico exacto de duración ilimitada. El gráfico cuenta lecturas válidas; los intervalos perdidos no se representan como puntos con fecha. El sensor usa su escala STANDARD heredada; esta revisión no valida una calibración ambiental ni calcula un AQI oficial.

El acceso local al panel y OTA sigue sin autenticación; el AP conserva la clave común y HTTPS conserva `setInsecure()` de la base. Las credenciales y destinos preexistentes se conservan. No se enviaron datos a los destinos reales durante las pruebas. La escritura OTA y las transferencias I2C siguen ocupando el ciclo principal durante su propia operación; las peticiones de telemetría y OAuth están separadas de él.

## Referencias de implementación

- [Registros y protocolo SEN0460 del fabricante](https://github.com/DFRobot/DFRobot_AirQualitySensor).
- [Ejemplo oficial de colas FreeRTOS de Espressif](https://github.com/espressif/arduino-esp32/tree/master/libraries/ESP32/examples/FreeRTOS/Queue).
- Código y revisión de TARS legacy 1.1.1 verificados en la ruta local indicada por el usuario.

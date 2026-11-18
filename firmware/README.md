# Firmware OTA air-2.0.1

[Descargar el .bin para OTA](https://raw.githubusercontent.com/1cfer/AirQualityMonitor/main/firmware/AirQualityMonitor_air-2.0.1_ESP32DA.bin)

Binario de aplicacion para **ESP32-WROOM-DA**, FQBN `esp32:esp32:esp32da`, nucleo 3.3.12 y particion de aplicacion de 1.310.720 bytes. Tamano del archivo: 1.187.088 bytes. No es una imagen merged de flash.

1. Boton conectado entre **GPIO27 y GND**. Mantener el boton 4 segundos para entrar al modo desarrollador.
2. Abrir la IP indicada por la OLED. En modo AP, conectarse a la red TARS del equipo y entrar a `http://192.168.4.1`.
3. En **Actualizacion OTA**, seleccionar `AirQualityMonitor_air-2.0.1_ESP32DA.bin` y pulsar **Subir Firmware**.
4. Esperar la respuesta y el reinicio, manteniendo la alimentacion. Al reiniciar, la pantalla de arranque indica `air-2.0.1`.

Se conserva la configuracion NVS del proyecto. Si el equipo usa otro modelo ESP32 o una particion sin espacio OTA suficiente, compilar `AIRQ/AIRQ.ino` para esa configuracion. Esta version compila y pasa pruebas nativas; aun no se ha validado en hardware fisico.

SHA-256:

```
1016a94232a335cc24cf26e054de63b46bb85468d4ac12884990cf2436a1d8ea
```

Tambien disponible en `SHA256SUMS.txt` y `BUILD_MANIFEST.json`.

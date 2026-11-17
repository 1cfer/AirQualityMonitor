# Firmware OTA air-2.0.0

[Descargar el .bin para OTA](https://raw.githubusercontent.com/1cfer/AirQualityMonitor/main/firmware/AirQualityMonitor_air-2.0.0_ESP32DA.bin)

Binario de aplicacion para **ESP32-WROOM-DA**, FQBN `esp32:esp32:esp32da`, nucleo 3.3.12 y particion de aplicacion de 1.310.720 bytes. Tamano del archivo: 1.187.088 bytes. No es una imagen merged de flash.

1. Mantener el boton 4 segundos para entrar al modo desarrollador.
2. Abrir la IP indicada por la OLED. En modo AP, conectarse a la red TARS del equipo y entrar a `http://192.168.4.1`.
3. En **Actualizacion OTA**, seleccionar `AirQualityMonitor_air-2.0.0_ESP32DA.bin` y pulsar **Subir Firmware**.
4. Esperar la respuesta y el reinicio, manteniendo la alimentacion. Al reiniciar, la pantalla de arranque indica `air-2.0.0`.

Se conserva la configuracion NVS del proyecto. Si el equipo usa otro modelo ESP32 o una particion sin espacio OTA suficiente, compilar `AIRQ/AIRQ.ino` para esa configuracion. Esta version compila y pasa pruebas nativas; aun no se ha validado en hardware fisico.

SHA-256:

```
3c957fc8ce1ccbad7d3b325dc8776c45f11b97fa4bed7a0e3e808ceb3b7ad1bd
```

Tambien disponible en `SHA256SUMS.txt` y `BUILD_MANIFEST.json`.

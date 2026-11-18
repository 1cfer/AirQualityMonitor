# MOREHA / airq1

Destino de calidad del aire, separado de `tars_moreha_04`:

- ID: `airq1`.
- Tipo NGSIv2: `AirQualityObserved`.
- Atributos Number: `pm1_0`, `pm2_5`, `pm10`, en ug/m3.
- Actualizaciones: `https://api.moreha.com.co/v2/entities/airq1/attrs`.
- Registro: `https://api.moreha.com.co/v2/entities`.
- Keyrock: `https://idm.moreha.com.co/oauth2/token`.
- OAuth por password grant y cabecera `Authorization: Bearer`.
- Boton: GPIO27 a GND.

## Registro con la primera lectura real

El firmware se autentica usando las credenciales guardadas en el ESP32 y envía el promedio pendiente por PATCH. Si el servidor devuelve 404, envía un POST de creación con `id`, `type` y las mismas tres concentraciones medidas. No se rellena la entidad con ceros ficticios. Si la creación devuelve 201, ese lote queda entregado y la pantalla muestra OK; las siguientes actualizaciones esperan normalmente HTTP 204.

No se consulta previamente si la entidad existe, no se registran entidades en cada envío y no se incorpora AMERI. Un error de conexión -1 no dispara la creación. Si el servidor rechaza el POST, se conservan todas las muestras para reintento.

Esta operación requiere un token válido y permiso para crear entidades en la aplicación MOREHA. Compilar o subir el firmware a GitHub no crea por sí solo la entidad en el servidor: la creación sucede cuando el equipo envía su primera lectura y MOREHA la acepta.

## Migración de la configuración anterior

En el primer arranque de air-2.0.2 se configura la URL pública con `airq1`, el token público, el envío directo y la autenticación habilitada. Se conserva el usuario, client ID, secreto y contraseña almacenados, además de WiFi e intervalos. Una marca NVS evita repetir la migración o sobrescribir cambios posteriores hechos en el panel.

En una instalación limpia, el secreto y la contraseña no vienen incrustados. Introducir los valores válidos de la aplicación Keyrock desde el panel local. El campo vacío en el formulario no muestra los secretos ya guardados.

## Resultados relevantes

| Resultado | Interpretación |
|---|---|
| 201 / OK | Creación y primera lectura aceptadas |
| 204 / OK | Actualización aceptada |
| HTTP -1 | No pudo establecerse la conexión al destino de datos |
| Fallo autenticacion / -1002 | No se consiguió un token válido |
| 401 o 403 | Revisar autenticación y autorización de la operación |
| 422 al crear | La entidad ya existe o hay una incompatibilidad de datos; las muestras se conservan |

Se verificaron DNS y TLS de los dominios públicos desde el equipo de desarrollo. El endpoint de datos respondió 401 sin token y reconoció la cabecera Bearer de prueba, sin enviar datos ni usar las credenciales privadas del equipo. La creación real sigue pendiente de confirmación desde el ESP32.

Referencia: [operación Create Entity de Orion NGSIv2](https://fiware-orion.readthedocs.io/en/3.12.0/orion-api.html#create-entity).

# Hito 5 (Variante Básica): Control Bidireccional MQTT (RPC), Telemetría y Alarma Simple en ThingsBoard

## Metadatos del Proyecto

* **Asignatura:** Sistemas Basados en Computador (SBC) — ETSISI UPM
* **Identificador de Grupo:** `SBC26T06` (**Grupo B**)
* **Miembros del Grupo:**
  * Ana Cuevas
  * Cristian Saiz
* **Plataforma Hardware:** ESP32-WROOM-32 (Xtensa Dual-Core 32-bit LX6)
* **Entorno y Framework:** ESP-IDF v5.3 / FreeRTOS Kernel
* **Plataforma IoT:** ThingsBoard Cloud (`eu.thingsboard.cloud`) / Protocolo MQTT

---

## 1. Descripción Técnica

Esta variante básica implementa el ciclo completo de instrumentación telemétrica y control bidireccional en tiempo real entre el SoC ESP32 y la plataforma ThingsBoard:

1. **Adquisición Analógica Calibrada:** Muestreo de la fotorresistencia LDR en el pin `GPIO34` (ADC1 canal 6) aplicando corrección de curvas de calibración eFuse (Lineal / Dos Puntos) para obtener lecturas deterministas en lux y milivoltios sin interferir con la radio Wi-Fi.
2. **Visualización Local en Hardware:** Mapeo simultáneo del nivel lumínico en el display de 7 segmentos (modo vúmetro por catálogo numérico) y representación textual en la pantalla OLED SSD1306 a través del bus I2C (`GPIO21` SDA, `GPIO22` SCL).
3. **Telemetría Periódica (MQTT):** Conexión Wi-Fi en modo Estación (STA) y publicación continua de la carga útil en formato JSON (`{"lux": X, "voltaje_mv": Y}`) sobre el topic estándar `v1/devices/me/telemetry` con QoS 1.
4. **Comunicación Bidireccional (MQTT RPC):** Suscripción al topic `v1/devices/me/rpc/request/+`. Recepción y decodificación del método RPC `setCadence` enviado desde los widgets de control del Dashboard (Knob/Slider), adaptando en caliente la cadencia del temporizador FreeRTOS de la tarea de adquisición sin provocar reinicios en el dispositivo.
5. **Procesamiento de Reglas y Alarma Simple (ThingsBoard):** Evaluación de la regla de umbral (`msg.lux < 300`) en el *Rule Engine* mediante TBEL/JavaScript, generando una alarma activa de tipo `Luz_Critica_Simple` y emitiendo una notificación automática vía REST API hacia el bot de Telegram con autolimpieza (`clear alarm`) al recuperar la iluminación normal.

---

## 2. Jerarquía de Archivos

```text
Hito5_Basico/
├── CMakeLists.txt              # Definición del proyecto CMake (hito5_basico)
├── sdkconfig                   # Configuración del SoC, FreeRTOS y subsistemas
├── components/
│   └── ssd1306/                # Driver desacoplado para display OLED por I2C
└── main/
    ├── CMakeLists.txt          # Registro de fuentes (SRCS) y dependencias (REQUIRES)
    ├── main.c                  # Orquestador: app_main, callback RPC y tarea periódica
    ├── include/
    │   ├── actuators.h         # API para control del display 7 segmentos y OLED
    │   ├── app_config.h        # Credenciales Wi-Fi, URI del Broker, Token y pines
    │   ├── mqtt_client_app.h   # API cliente MQTT, suscripciones RPC y publicación
    │   ├── sensor_adc.h        # API de calibración y muestreo de la LDR
    │   └── wifi_sta.h          # API de inicialización y eventos de red Wi-Fi
    └── src/
        ├── actuators.c         # Decodificación de dígitos y refresco de pantalla I2C
        ├── mqtt_client_app.c   # Manejador de eventos MQTT y enrutador de comandos RPC
        ├── sensor_adc.c        # Lectura oneshot del ADC y conversión a lux
        └── wifi_sta.c          # Máquina de estados de conexión STA con reconexión
```

## 3. Asignación de Pines e Interfaces Hardware

| Periférico / Componente | Línea / Pin ESP32 | Protocolo / Modo | Función / Descripción |
| :--- | :---: | :---: | :--- |
| **Sensor LDR (Divisor de tensión)** | `GPIO 34` | ADC1 (CH6) | Entrada analógica con calibración eFuse |
| **OLED SSD1306 (SDA)** | `GPIO 21` | I2C Master | Línea de datos (400 kHz) |
| **OLED SSD1306 (SCL)** | `GPIO 22` | I2C Master | Línea de reloj (400 kHz) |
| **Display 7 Segmentos (A)** | `GPIO 13` | Salida GPIO | Segmento A |
| **Display 7 Segmentos (B)** | `GPIO 12` | Salida GPIO | Segmento B |
| **Display 7 Segmentos (C)** | `GPIO 14` | Salida GPIO | Segmento C |
| **Display 7 Segmentos (D)** | `GPIO 27` | Salida GPIO | Segmento D |
| **Display 7 Segmentos (E)** | `GPIO 26` | Salida GPIO | Segmento E |
| **Display 7 Segmentos (F)** | `GPIO 25` | Salida GPIO | Segmento F |
| **Display 7 Segmentos (G)** | `GPIO 33` | Salida GPIO | Segmento G |

---

## 4. Instrucciones de Compilación y Monitorización

Desde la terminal, situarse en el directorio del proyecto básico y ejecutar:

```bash
# 1. Acceso a la carpeta de la variante básica
cd Hito5_Basico

# 2. Compilación del proyecto completo
idf.py build

# 3. Flasheo en la placa y apertura del monitor serie
idf.py -p /dev/ttyUSB0 flash monitor

## 5. Protocolo de Verificación Funcional

1. **Arranque y Conectividad:** Verificar en el monitor serie que se obtiene dirección IP por DHCP y que el cliente MQTT se suscribe con éxito a `v1/devices/me/rpc/request/+`.
2. **Monitorización en Dashboard:** Observar cómo el widget *Radial Gauge* se actualiza al ritmo establecido por la cadencia por defecto (2 segundos).
3. **Validación del RPC Bidireccional:** Modificar el valor en el widget *Knob Control* (ej. a 5 s). Comprobar en consola la traza:
   ```text
   Comando RPC entrante [ID=...]: {"method":"setCadence","params":5}
   Cadencia actualizada a 5 s desde ThingsBoard (RPC: setCadence)
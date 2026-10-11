# Hito 5 (Variante Avanzada): Gestión Energética (Modem-Sleep), Control Bidireccional RPC, Reglas Complejas en ThingsBoard y Alertas en Telegram

## Metadatos del Proyecto

* **Asignatura:** Sistemas Basados en Computador (SBC) — ETSISI UPM
* **Identificador de Grupo:** `SBC26T06` (**Grupo B**)
* **Miembros del Grupo:**
  * Ana Cuevas
  * Cristian Saiz
* **Plataforma Hardware:** ESP32-WROOM-32 (Xtensa Dual-Core 32-bit LX6)
* **Entorno y Framework:** ESP-IDF v5.3 / FreeRTOS Kernel
* **Plataforma IoT y Notificaciones:** ThingsBoard Cloud (`eu.thingsboard.cloud`) / Telegram Bot API

---

## 1. Descripción Técnica y Política Energética

Esta variante avanzada amplía las capacidades de instrumentación y control bidireccional del sistema mediante la integración de políticas de ahorro de energía y procesamiento multivariable de eventos:

1. **Gestión Energética (Power Management):**
   * **Política Adoptada:** Concurrencia entre bajo consumo y respuesta en tiempo real. En lugar de un *Deep Sleep* que desconectaría la sesión TCP/MQTT e impediría recibir órdenes remotas, se implementa **Modem-Sleep** (`WIFI_PS_MIN_MODEM`) mediante las APIs nativas de ESP-IDF (`esp_wifi_set_ps`). El transceptor RF apaga su circuitería entre balizas DTIM del punto de acceso Wi-Fi, reduciendo el consumo medio en más de un 75% (~20–30 mA frente a los ~180 mA de radio continua) preservando el socket MQTT y la reactividad instantánea ante comandos RPC.
   * **Módulo `power_mgmt`:** Proporciona además rutinas de suspensión temporizada en *Light Sleep* (`esp_light_sleep_start()`) con retención de RAM y arranque determinista por temporizador RTC.

2. **Instrumentación Calibrada y Actuadores:** Muestreo de la LDR en `GPIO34` (ADC1 canal 6) con calibración eFuse, representación del nivel en display de 7 segmentos y visualización de telemetría y estado en la pantalla OLED SSD1306 (I2C).

3. **Control Bidireccional (RPC por MQTT):** Recepción asíncrona de solicitudes remotas desde ThingsBoard (`v1/devices/me/rpc/request/+`) para el método `setCadence`. La tarea FreeRTOS actualiza su periodo de bloqueo dinámicamente, reflejando el cambio en el display de 7 segmentos y la pantalla OLED.

4. **Reglas Complejas y Notificaciones en Telegram:**
   * **Condición Simple:** Disparo de alarma por umbral único de luz (`msg.lux < 300`).
   * **Condición Compleja:** Regla multivariable evaluada en el *Rule Engine* mediante TBEL/JavaScript que exige la coincidencia simultánea de dos magnitudes físicas: caída crítica de iluminación (`msg.lux < 200`) y confirmación de baja tensión en el divisor del sensor (`msg.voltaje_mv < 1200`).
   * **Integración con Telegram Bot API:** Petición REST HTTP POST (`rest api call`) contra el endpoint de Telegram formateando dinámicamente los valores medidos (`msg.details.lux` y `msg.details.voltaje_mv`), con autolimpieza (`clear alarm`) de ambas alarmas cuando las condiciones retornan a los rangos normales.

---

## 2. Jerarquía de Archivos

```text
Hito5_Avanzado/
├── CMakeLists.txt              # Definición del proyecto CMake (hito5_avanzado)
├── sdkconfig                   # Configuración del SoC, FreeRTOS y subsistemas
├── components/
│   └── ssd1306/                # Driver desacoplado para display OLED por I2C
└── main/
    ├── CMakeLists.txt          # Registro de fuentes y dependencias (esp_pm, json, etc.)
    ├── main.c                  # Orquestador con inicialización de bajo consumo
    ├── include/
    │   ├── actuators.h         # Control de display 7 segmentos y pantalla OLED
    │   ├── app_config.h        # Credenciales de red, broker MQTT, topics y pines
    │   ├── mqtt_client_app.h   # Enlace MQTT, suscripción RPC y telemetría
    │   ├── power_mgmt.h        # API de gestión energética (Modem-Sleep y Sleep modes)
    │   ├── sensor_adc.h        # Calibración eFuse y lectura de la LDR
    │   └── wifi_sta.h          # Conexión Wi-Fi en modo Estación (STA)
    └── src/
        ├── actuators.c         # Decodificación BCD y dibujado en buffer SSD1306
        ├── mqtt_client_app.c   # Gestión de eventos MQTT y despacho de RPC
        ├── power_mgmt.c        # Configuración de Modem-Sleep y timers RTC
        ├── sensor_adc.c        # Muestreo analógico y conversión a lux/mV
        └── wifi_sta.c          # Máquina de estados Wi-Fi y reconexión automática
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

Desde el terminal, situarse en el directorio del proyecto avanzado y ejecutar:

# 1. Acceso a la carpeta de la variante avanzada
cd Hito5_Avanzado

# 2. Compilación del proyecto completo
idf.py build

# 3. Flasheo en la placa y apertura del monitor serie
idf.py -p /dev/ttyUSB0 flash monitor

---

## 5. Protocolo de Verificación Funcional

1. **Inicialización y Confirmación de Bajo Consumo:**
   * Al reiniciar el ESP32, verificar en la consola serie la activación del subsistema de energía:
     POWER_MGMT: Arranque inicial: Power-on reset o reinicio manual.
     POWER_MGMT: Politica de ahorro: Modem-Sleep activado (WIFI_PS_MIN_MODEM).
   * Confirmar la obtención de IP vía DHCP y la suscripción exitosa al topic RPC v1/devices/me/rpc/request/+.

2. **Control Bidireccional (RPC) bajo Modem-Sleep:**
   * Accionar el widget Knob Control en el Dashboard de ThingsBoard seleccionando un nuevo periodo (ej. 5 segundos).
   * Constatar en la salida serie la recepción del comando setCadence, la actualización inmediata en pantalla OLED (Cadencia: 5 s) y el ajuste de cadencia en la tarea periódica sin pérdida de paquetes TCP/MQTT.

3. **Disparo de Alarma Simple (Umbral Único):**
   * Reducir levemente la iluminación ambiente sobre la LDR (200–299 lux).
   * Verificar en el chat de Telegram la recepción del mensaje:
     ⚠️ [ALERTA HITO 5] Alarma de luminosidad baja activada...

4. **Disparo de Alarma Compleja (Multivariable Concurrente):**
   * Bloquear completamente la incidencia lumínica sobre el sensor con el dedo.
   * Al cumplirse simultáneamente lux < 200 y voltaje_mv < 1200, constatar la llegada inmediata de la alerta a Telegram:
     🚨 [ALERTA COMPLEJA - HITO 5]
     Condición multivariable disparada en: ESP32_Hito5_SBC26T06
     • Luminosidad: <valor> lux
     • Tensión ADC: <valor> mV
     Estado: Oscuridad crítica confirmada por doble magnitud.

5. **Autolimpieza y Ciclo de Vida de Alarmas:**
   * Retirar la obstrucción del sensor permitiendo que la luz retorne a niveles normales (> 300 lux).
   * Confirmar en ThingsBoard (Entities -> Devices -> Alarms) que ambas alarmas transicionan automáticamente a estado resuelto (Cleared), quedando el sistema listo para nuevos disparos sin intervención manual.
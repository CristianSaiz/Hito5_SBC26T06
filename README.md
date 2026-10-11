# Hito 5: Reglas, Alarmas con Telegram, Comunicación Bidireccional (RPC) y Gestión Energética en ThingsBoard y ESP32

## Metadatos del Proyecto

* **Asignatura:** Sistemas Basados en Computador (SBC) — ETSISI UPM
* **Identificador de Grupo:** `SBC26T06` (**Grupo B**)
* **Miembros del Grupo:**
  * Ana Cuevas
  * Cristian Saiz
* **Plataforma Hardware:** ESP32-WROOM-32 (Xtensa Dual-Core 32-bit LX6)
* **Entorno y Framework:** ESP-IDF v5.3 / FreeRTOS Kernel
* **Plataforma IoT:** ThingsBoard Cloud (`eu.thingsboard.cloud`) / Protocolo MQTT
* **Servicio de Notificación:** Telegram Bot API
* **Ruta de Entrega en SharePoint:** `\Documentos\Reto5\GrupoB`
* **Vídeo Demostrativo:** [Enlace a la demostración en SharePoint / Stream]

---

## 1. Descripción General del Hito

Este proyecto implementa la integración completa de un nodo sensor y actuador IoT basado en el microcontrolador ESP32 con la plataforma en la nube ThingsBoard, cumpliendo los requisitos funcionales del Hito 5:

1. **Adquisición Analógica Calibrada:** Muestreo periódico de una fotorresistencia (LDR) en el canal seguro `ADC1_CH6` (GPIO 34), con linealización por curvas de calibración eFuse para calcular iluminancia (`lux`) y caída de tensión (`voltaje_mv`).
2. **Visualización Local en Hardware:** Mapeo simultáneo en un display de 7 segmentos (modo vúmetro numérico) y representación textual en un display OLED SSD1306 sobre el bus I2C (GPIO 21 SDA, GPIO 22 SCL).
3. **Comunicación Bidireccional por MQTT (RPC):** Suscripción al topic `v1/devices/me/rpc/request/+`. El sistema procesa peticiones de control remoto (`setCadence`) enviadas desde el Dashboard web de ThingsBoard o la app móvil, adaptando de forma inmediata la periodicidad de la tarea de FreeRTOS sin provocar reinicios en caliente.
4. **Reglas y Alarmas con Notificación en Telegram:**
   * **Condición Simple:** Generación de alarma por umbral directo de iluminancia (`msg.lux < 300`).
   * **Condición Compleja:** Regla combinada multivariable en el Rule Engine (TBEL) que exige simultáneamente caída de luz (`msg.lux < 200`) y caída de tensión (`msg.voltaje_mv < 1200`).
   * **Integración REST API:** Envío automático de notificaciones formateadas al bot de Telegram y resolución automática de alarmas (`clear alarm`) al recuperar los niveles nominales de luz.
5. **Gestión de Consumo (Power Management):** Justificación e integración de la política de ahorro energético con **Modem-Sleep** (`WIFI_PS_MIN_MODEM`) en la variante avanzada, reduciendo la corriente media del sistema sin desconectar la sesión MQTT ni perder la reactividad ante comandos RPC.

El repositorio se estructura en dos proyectos independientes:
* **`Hito5_Basico`**: Enfoque centrado en la telemetría periódica MQTT, control bidireccional por RPC y regla simple de alarma en ThingsBoard.
* **`Hito5_Avanzado`**: Amplía el sistema integrando la política de gestión energética (Modem-Sleep), regla compleja multivariable en ThingsBoard y formateo dinámico de alertas en Telegram.

---

## 2. Estructura del Repositorio

El repositorio está organizado conforme a las directrices de empaquetado de la UPM, conteniendo dos subproyectos CMake completos y desacoplados:

Hito5_SBC26T06/
├── README.md                   # Documentación técnica general y especificaciones
├── Hito5_Basico/               # Proyecto completo de la variante básica
│   ├── CMakeLists.txt          # Script CMake del proyecto (hito5_basico)
│   ├── sdkconfig               # Opciones de compilación y parámetros de FreeRTOS
│   ├── README.md               # Memoria técnica y pruebas del Hito 5 Básico
│   ├── components/
│   │   └── ssd1306/            # Driver local para display OLED por I2C
│   └── main/
│       ├── CMakeLists.txt      # Definición de fuentes (SRCS) y dependencias (REQUIRES)
│       ├── main.c              # Punto de entrada (app_main) y tarea de telemetría
│       ├── include/            # Cabeceras modulares (actuators, app_config, mqtt, adc, wifi)
│       └── src/                # Implementación modular en lenguaje C
│
└── Hito5_Avanzado/             # Proyecto completo de la variante avanzada
    ├── CMakeLists.txt          # Script CMake del proyecto (hito5_avanzado)
    ├── sdkconfig               # Opciones de compilación de la variante avanzada
    ├── README.md               # Memoria técnica, política energética y pruebas
    ├── components/
    │   └── ssd1306/            # Driver local para display OLED por I2C
    └── main/
        ├── CMakeLists.txt      # Registro de dependencias (incluye esp_pm y json)
        ├── main.c              # Orquestador con control de energía y ráfagas
        ├── include/            # Cabeceras (incluye power_mgmt.h)
        └── src/                # Implementaciones (incluye power_mgmt.c)

---

## 3. Asignación de Pines e Interfaces Hardware

| Periférico / Componente | Pin ESP32 | Modo / Protocolo | Función |
| :--- | :---: | :---: | :--- |
| **Sensor LDR (Divisor resistivo)** | `GPIO 34` | ADC1 Canal 6 | Muestreo analógico con calibración eFuse |
| **Display OLED SSD1306 (SDA)** | `GPIO 21` | I2C Master | Línea de datos serie (400 kHz) |
| **Display OLED SSD1306 (SCL)** | `GPIO 22` | I2C Master | Línea de reloj serie (400 kHz) |
| **Display 7 Segmentos (A)** | `GPIO 13` | Salida GPIO | Segmento A |
| **Display 7 Segmentos (B)** | `GPIO 12` | Salida GPIO | Segmento B |
| **Display 7 Segmentos (C)** | `GPIO 14` | Salida GPIO | Segmento C |
| **Display 7 Segmentos (D)** | `GPIO 27` | Salida GPIO | Segmento D |
| **Display 7 Segmentos (E)** | `GPIO 26` | Salida GPIO | Segmento E |
| **Display 7 Segmentos (F)** | `GPIO 25` | Salida GPIO | Segmento F |
| **Display 7 Segmentos (G)** | `GPIO 33` | Salida GPIO | Segmento G |

---

## 4. Compilación y Ejecución

### Opción A: Compilar y Ejecutar la Variante Básica
cd Hito5_Basico
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor

### Opción B: Compilar y Ejecutar la Variante Avanzada
cd Hito5_Avanzado
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor

(Para salir de la consola de monitorización serie de ESP-IDF, pulsar Ctrl + ])

---

## 5. Protocolo de Verificación y Guion Demostrativo

1. **Arranque y Conectividad:** Verificar en el monitor serie la inicialización de periféricos, conexión a la red Wi-Fi y confirmación de enlace con el broker MQTT de ThingsBoard. En la variante avanzada, comprobar la traza de activación del modo de bajo consumo:
   POWER_MGMT: Politica de ahorro: Modem-Sleep activado (WIFI_PS_MIN_MODEM).
2. **Telemetría e Interfaz Local:** Confirmar que la variación de luminosidad sobre la LDR actualiza simultáneamente el display de 7 segmentos, la pantalla OLED y el widget de aguja (Radial Gauge) en el Dashboard de ThingsBoard.
3. **Control Bidireccional (RPC):** Modificar la cadencia desde el control Knob del Dashboard (ej. cambiando de 2 s a 5 s). Verificar la recepción del comando `setCadence`, el refresco en la pantalla OLED (`Cadencia: 5 s`) y la adaptación de la frecuencia de publicación de telemetría sin reinicios.
4. **Disparo de Alarma Simple:** Reducir parcialmente la luz sobre el sensor (< 300 lux) y confirmar la notificación automática de alerta en Telegram.
5. **Disparo de Alarma Compleja:** Bloquear totalmente la luz con el dedo (< 200 lux y < 1200 mV) y comprobar la recepción de la alerta multivariable en Telegram con los valores reales medidos.
6. **Autolimpieza:** Retirar el dedo para permitir la iluminación normal (> 300 lux); verificar que ambas alarmas transicionan a estado limpio (`Cleared`) en ThingsBoard.
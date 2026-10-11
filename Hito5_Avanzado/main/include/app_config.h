#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// --- Credenciales de Red Wi-Fi ---
// Ajustar a tu red local (recordar no subir valores reales a repositorios publicos)
#define WIFI_SSID               "TU_RED_WIFI"
#define WIFI_PASS               "TU_PASSWORD"
#define WIFI_MAXIMUM_RETRY      5

// --- Broker MQTT y ThingsBoard ---
#define TB_BROKER_URI           "mqtt://eu.thingsboard.cloud:1883"
#define TB_DEVICE_TOKEN         "TU_DEVICE_TOKEN"

// Topics estandar de ThingsBoard
#define TB_TELEMETRY_TOPIC      "v1/devices/me/telemetry"
#define TB_RPC_REQUEST_TOPIC    "v1/devices/me/rpc/request/+"
#define TB_RPC_RESPONSE_PREFIX  "v1/devices/me/rpc/response/"

// --- Parametros de Muestreo y Control Bidireccional ---
// Cadencia inicial y limites para evitar desbordamientos o saturacion del broker
#define DEFAULT_SAMPLE_PERIOD_S 2
#define MIN_SAMPLE_PERIOD_S     1
#define MAX_SAMPLE_PERIOD_S     30

// --- Asignacion de Pines (Hardware SBC Entrenador) ---
// Entrada analogica LDR (ADC1_CH6 en GPIO34 para evitar conflictos con Wi-Fi)
#define SENSOR_LDR_GPIO         34

// Bus I2C para pantalla OLED SSD1306
#define I2C_MASTER_SDA_IO       21
#define I2C_MASTER_SCL_IO       22
#define I2C_MASTER_FREQ_HZ      400000

// Display de 7 segmentos (Catodo Comun)
#define SEG_A_GPIO              13
#define SEG_B_GPIO              12
#define SEG_C_GPIO              14
#define SEG_D_GPIO              27
#define SEG_E_GPIO              26
#define SEG_F_GPIO              25
#define SEG_G_GPIO              33

#endif // APP_CONFIG_H
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "app_config.h"
#include "wifi_sta.h"
#include "sensor_adc.h"
#include "actuators.h"
#include "mqtt_client_app.h"

static const char *TAG = "MAIN_HITO5_BASICO";

// Variable compartida para la cadencia de muestreo (modificable via RPC)
static volatile uint32_t s_sample_period_s = DEFAULT_SAMPLE_PERIOD_S;
static char s_last_rpc_name[32] = "";

/**
 * @brief Callback invocado cuando ThingsBoard envia una orden RPC de cambio de periodo
 */
static void on_rpc_cadence_changed(uint32_t new_period_s, const char *method_name)
{
    s_sample_period_s = new_period_s;
    if (method_name != NULL) {
        strncpy(s_last_rpc_name, method_name, sizeof(s_last_rpc_name) - 1);
        s_last_rpc_name[sizeof(s_last_rpc_name) - 1] = '\0';
    }
    ESP_LOGI(TAG, "Cadencia actualizada a %lu s desde ThingsBoard (RPC: %s)",
             (unsigned long)s_sample_period_s, s_last_rpc_name);
}

/**
 * @brief Tarea periodica de instrumentacion, refresco y publicacion telemetrica
 */
static void telemetry_task(void *pvParameters)
{
    ldr_reading_t ldr_data;
    char telemetry_json[128];

    while (1) {
        // 1. Muestreo del sensor de luz (LDR en GPIO34)
        if (sensor_adc_read(&ldr_data) == ESP_OK) {
            ESP_LOGI(TAG, "Lectura LDR -> Lux: %.1f | Tension: %d mV | Raw: %d",
                     ldr_data.lux, ldr_data.voltage_mv, ldr_data.raw);

            // 2. Refresco de indicadores locales (OLED y 7 segmentos)
            bool tb_connected = mqtt_client_app_is_connected();
            actuators_update_oled(ldr_data.lux, s_sample_period_s, tb_connected, s_last_rpc_name);

            // 3. Publicacion MQTT si la conexion esta activa
            if (tb_connected) {
                snprintf(telemetry_json, sizeof(telemetry_json),
                         "{\"lux\":%.2f,\"voltaje_mv\":%d}",
                         ldr_data.lux, ldr_data.voltage_mv);

                mqtt_client_app_publish_telemetry(telemetry_json);
            }
        } else {
            ESP_LOGW(TAG, "No se pudo obtener lectura del ADC");
        }

        // Retardo dinamico segun la cadencia actual fijada por RPC
        vTaskDelay(pdMS_TO_TICKS(s_sample_period_s * 1000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando SBC Hito 5 - Variante Basica");

    // 1. Inicializar almacenamiento no volatil (NVS)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Inicializar actuadores e instrumentacion fisica
    ESP_ERROR_CHECK(actuators_init());
    ESP_ERROR_CHECK(sensor_adc_init());

    // 3. Conectar a la red Wi-Fi
    ESP_LOGI(TAG, "Estableciendo enlace Wi-Fi con el AP...");
    ret = wifi_sta_init(WIFI_SSID, WIFI_PASS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error critico: no se pudo asociar a la red Wi-Fi");
        return;
    }

    // 4. Iniciar cliente MQTT con ThingsBoard y registrar callback RPC
    ESP_LOGI(TAG, "Conectando al broker MQTT de ThingsBoard...");
    ESP_ERROR_CHECK(mqtt_client_app_start(on_rpc_cadence_changed));

    // 5. Lanzar tarea principal de telemetria
    xTaskCreatePinnedToCore(
        telemetry_task,
        "telemetry_task",
        4096,
        NULL,
        5,
        NULL,
        1 // Core 1 para instrumentacion, dejando Core 0 para la pila de red
    );
}
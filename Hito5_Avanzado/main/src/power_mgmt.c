#include "power_mgmt.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "esp_log.h"

static const char *TAG = "POWER_MGMT";

esp_err_t power_mgmt_init(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Reactivacion: Despertado por temporizador RTC.");
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            ESP_LOGI(TAG, "Arranque inicial: Power-on reset o reinicio manual.");
            break;
    }
    return ESP_OK;
}

esp_err_t power_mgmt_enable_modem_sleep(void)
{
    // Modem-Sleep: el transceptor RF se apaga entre balizas DTIM del punto de acceso.
    // Permite ahorrar ~100 mA manteniendo el socket MQTT activo para comandos RPC entrantes.
    esp_err_t ret = esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Politica de ahorro: Modem-Sleep activado (WIFI_PS_MIN_MODEM).");
    } else {
        ESP_LOGW(TAG, "Fallo al activar Modem-Sleep: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t power_mgmt_enter_light_sleep(uint32_t sleep_time_ms)
{
    if (sleep_time_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    uint64_t sleep_time_us = (uint64_t)sleep_time_ms * 1000ULL;
    esp_err_t ret = esp_sleep_enable_timer_wakeup(sleep_time_us);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error configurando timer para Light Sleep: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Entrando en Light Sleep durante %lu ms (RAM retenida)...", (unsigned long)sleep_time_ms);
    
    // Bloqueo y suspension del reloj digital principal
    ret = esp_light_sleep_start();
    
    ESP_LOGI(TAG, "CPU reactivada tras Light Sleep.");
    return ret;
}

void power_mgmt_enter_deep_sleep(uint32_t sleep_time_s)
{
    ESP_LOGI(TAG, "Entrando en Deep Sleep durante %lu segundos...", (unsigned long)sleep_time_s);
    esp_sleep_enable_timer_wakeup((uint64_t)sleep_time_s * 1000000ULL);
    esp_deep_sleep_start();
}
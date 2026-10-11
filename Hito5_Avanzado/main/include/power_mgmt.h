#ifndef POWER_MGMT_H
#define POWER_MGMT_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa el subsistema de gestion energetica y detecta la causa de despertar.
 * 
 * @return esp_err_t ESP_OK si la inicializacion fue correcta.
 */
esp_err_t power_mgmt_init(void);

/**
 * @brief Configura el modo Modem-Sleep en el stack Wi-Fi (WIFI_PS_MIN_MODEM).
 *        Reduce el consumo de RF manteniendo la sesion TCP/MQTT activa.
 * 
 * @return esp_err_t ESP_OK si se configuro con exito.
 */
esp_err_t power_mgmt_enable_modem_sleep(void);

/**
 * @brief Introduce al ESP32 en Light Sleep durante un intervalo de tiempo.
 *        Preserva el contenido de la memoria RAM y el estado de los perifericos.
 * 
 * @param sleep_time_ms Tiempo de suspension en milisegundos.
 * @return esp_err_t ESP_OK tras despertar por temporizador.
 */
esp_err_t power_mgmt_enter_light_sleep(uint32_t sleep_time_ms);

/**
 * @brief Introduce al ESP32 en Deep Sleep durante un intervalo determinado.
 *        Se apagan CPU, RAM y perifericos; despierta como un reinicio limpio.
 * 
 * @param sleep_time_s Tiempo de sueno en segundos.
 */
void power_mgmt_enter_deep_sleep(uint32_t sleep_time_s);

#ifdef __cplusplus
}
#endif

#endif // POWER_MGMT_H
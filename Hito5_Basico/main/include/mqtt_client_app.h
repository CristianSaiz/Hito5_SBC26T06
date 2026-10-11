#ifndef MQTT_CLIENT_APP_H
#define MQTT_CLIENT_APP_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback invocado cuando ThingsBoard envia un comando RPC para modificar la cadencia.
 * 
 * @param new_period_s Nueva cadencia solicitada en segundos.
 * @param method_name Nombre del metodo RPC recibido (para mostrar en trazas u OLED).
 */
typedef void (*rpc_cadence_callback_t)(uint32_t new_period_s, const char *method_name);

/**
 * @brief Inicializa el cliente MQTT, conecta a ThingsBoard y se suscribe a los topics RPC.
 * 
 * @param on_cadence Callback que recibira las ordenes de cambio de periodo.
 * @return esp_err_t ESP_OK si el cliente inicio correctamente.
 */
esp_err_t mqtt_client_app_start(rpc_cadence_callback_t on_cadence);

/**
 * @brief Publica una cadena JSON en el topic de telemetria (v1/devices/me/telemetry).
 * 
 * @param json_payload Cadena con formato JSON (ej: "{\"lux\":120.5}").
 * @return esp_err_t ESP_OK si se encolo la publicacion.
 */
esp_err_t mqtt_client_app_publish_telemetry(const char *json_payload);

/**
 * @brief Consulta si la sesion MQTT esta establecida con ThingsBoard.
 * 
 * @return true si esta conectado.
 */
bool mqtt_client_app_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // MQTT_CLIENT_APP_H
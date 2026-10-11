#ifndef ACTUATORS_H
#define ACTUATORS_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa los buses y pines de hardware de la pantalla OLED y el display de 7 segmentos.
 * 
 * @return esp_err_t ESP_OK si la inicializacion fue correcta.
 */
esp_err_t actuators_init(void);

/**
 * @brief Actualiza la pantalla OLED con las metricas en vivo y el estado de ThingsBoard.
 * 
 * @param lux Valor actual de iluminacion medido por la LDR.
 * @param period_s Cadencia de envio configurada actualmente (segundos).
 * @param tb_connected true si la sesion MQTT con ThingsBoard esta activa.
 * @param last_rpc_method Nombre del ultimo comando RPC ejecutado (o NULL si ninguno).
 */
void actuators_update_oled(float lux, uint32_t period_s, bool tb_connected, const char *last_rpc_method);

/**
 * @brief Muestra un digito numerico (0-9) en el display de 7 segmentos (catodo comun).
 * 
 * @param digit Digito a representar (si es mayor a 9 o negativo, se apaga el display).
 */
void actuators_set_7seg_digit(int digit);

#ifdef __cplusplus
}
#endif

#endif // ACTUATORS_H
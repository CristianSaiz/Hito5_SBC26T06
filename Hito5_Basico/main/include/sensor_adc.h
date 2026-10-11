#ifndef SENSOR_ADC_H
#define SENSOR_ADC_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int raw;             // Lectura bruta ADC (0-4095)
    int voltage_mv;      // Tension calibrada en mV
    float lux;           // Estimacion en luxes
} ldr_reading_t;

/**
 * @brief Inicializa el driver ADC1 en modo oneshot y calibra la curva de respuesta.
 * 
 * @return esp_err_t ESP_OK si la inicializacion fue correcta.
 */
esp_err_t sensor_adc_init(void);

/**
 * @brief Realiza un muestreo de la LDR con sobremuestreo y calcula los luxes.
 * 
 * @param reading Puntero a la estructura donde almacenar los resultados.
 * @return esp_err_t ESP_OK si la lectura es valida.
 */
esp_err_t sensor_adc_read(ldr_reading_t *reading);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_ADC_H
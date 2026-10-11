#include <math.h>
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "sensor_adc.h"
#include "app_config.h"

static const char *TAG = "SENSOR_ADC";

static adc_oneshot_unit_handle_t s_adc_handle = NULL;
static adc_cali_handle_t s_cali_handle = NULL;
static bool s_calibrated = false;

// GPIO 34 corresponde a ADC1_CHANNEL_6 en ESP32
#define LDR_ADC_CHANNEL ADC_CHANNEL_6
#define OVERSAMPLING_SAMPLES 16

static bool init_calibration(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    esp_err_t ret = ESP_FAIL;
    bool cali_ok = false;

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, out_handle);
    if (ret == ESP_OK) {
        cali_ok = true;
    }
#endif

    if (!cali_ok) {
        ESP_LOGW(TAG, "No se pudo aplicar calibracion eFuse Line-Fitting, usando escala por defecto");
    }
    return cali_ok;
}

esp_err_t sensor_adc_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &s_adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al crear la unidad oneshot del ADC");
        return ret;
    }

    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_oneshot_config_channel(s_adc_handle, LDR_ADC_CHANNEL, &chan_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al configurar el canal ADC");
        return ret;
    }

    s_calibrated = init_calibration(ADC_UNIT_1, LDR_ADC_CHANNEL, ADC_ATTEN_DB_12, &s_cali_handle);
    ESP_LOGI(TAG, "ADC inicializado correctamente en GPIO %d", SENSOR_LDR_GPIO);
    return ESP_OK;
}

esp_err_t sensor_adc_read(ldr_reading_t *reading)
{
    if (s_adc_handle == NULL || reading == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    int raw_accum = 0;
    int raw_val = 0;

    for (int i = 0; i < OVERSAMPLING_SAMPLES; i++) {
        esp_err_t ret = adc_oneshot_read(s_adc_handle, LDR_ADC_CHANNEL, &raw_val);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error durante la lectura del canal ADC");
            return ret;
        }
        raw_accum += raw_val;
    }

    reading->raw = raw_accum / OVERSAMPLING_SAMPLES;

    if (s_calibrated && s_cali_handle != NULL) {
        adc_cali_raw_to_voltage(s_cali_handle, reading->raw, &reading->voltage_mv);
    } else {
        reading->voltage_mv = (reading->raw * 3300) / 4095;
    }

    // Calculo aproximado de luxes a partir del divisor de tension
    // Mayor luz -> menor resistencia LDR -> mayor tension leida en pull-down
    float v_in = (float)reading->voltage_mv / 1000.0f;
    if (v_in < 0.05f) {
        reading->lux = 0.0f;
    } else {
        float lux_val = (v_in / 3.3f) * 1000.0f;
        reading->lux = (lux_val > 1000.0f) ? 1000.0f : lux_val;
    }

    return ESP_OK;
}
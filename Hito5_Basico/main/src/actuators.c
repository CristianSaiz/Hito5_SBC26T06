#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "actuators.h"
#include "app_config.h"
#include "ssd1306.h"

static const char *TAG = "ACTUATORS";

static SSD1306_t s_oled_dev;
static bool s_oled_ready = false;

// Pines fisicos ordenados de segmento A a segmento G
static const gpio_num_t s_seg_pins[7] = {
    SEG_A_GPIO, SEG_B_GPIO, SEG_C_GPIO, SEG_D_GPIO,
    SEG_E_GPIO, SEG_F_GPIO, SEG_G_GPIO
};

// Mascara de 64 bits (ULL indispensable para GPIO 33)
#define SEGMENTS_PIN_MASK ( \
    (1ULL << SEG_A_GPIO) | (1ULL << SEG_B_GPIO) | \
    (1ULL << SEG_C_GPIO) | (1ULL << SEG_D_GPIO) | \
    (1ULL << SEG_E_GPIO) | (1ULL << SEG_F_GPIO) | \
    (1ULL << SEG_G_GPIO)   \
)

// Tabla de verdad BCD a 7 segmentos (Bit 0 = A, ..., Bit 6 = G)
static const uint8_t s_digit_map[10] = {
    0b00111111, // 0: A, B, C, D, E, F
    0b00000110, // 1: B, C
    0b01011011, // 2: A, B, D, E, G
    0b01001111, // 3: A, B, C, D, G
    0b01100110, // 4: B, C, F, G
    0b01101101, // 5: A, C, D, F, G
    0b01111101, // 6: A, C, D, E, F, G
    0b00000111, // 7: A, B, C
    0b01111111, // 8: A, B, C, D, E, F, G
    0b01101111  // 9: A, B, C, D, F, G
};

static void init_7segment_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = SEGMENTS_PIN_MASK,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Apagar todos los segmentos inicialmente
    actuators_set_7seg_digit(-1);
}

void actuators_set_7seg_digit(int digit)
{
    // En Anodo Comun: aplicar nivel 1 (3.3V) a los catodos APAGA los diodos
    if (digit < 0 || digit > 9) {
        for (int i = 0; i < 7; i++) {
            gpio_set_level(s_seg_pins[i], 1);
        }
        return;
    }

    uint8_t mask = s_digit_map[digit];
    for (int i = 0; i < 7; i++) {
        // Logica activa en bajo: bit a 1 -> 0V (LED enciende); bit a 0 -> 3.3V (apaga)
        int bit_active = (mask >> i) & 0x01;
        gpio_set_level(s_seg_pins[i], bit_active ? 0 : 1);
    }
}

esp_err_t actuators_init(void)
{
    init_7segment_gpio();

    // Inicializar bus I2C y display OLED SSD1306 (128x64)
    i2c_master_init(&s_oled_dev, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, -1);
    esp_err_t ret = i2c_init(&s_oled_dev, 128, 64);
    if (ret == ESP_OK) {
        s_oled_ready = true;
        ssd1306_clear_screen(&s_oled_dev, false);
        ssd1306_contrast(&s_oled_dev, 0xff);
        ssd1306_display_text(&s_oled_dev, 0, "  SBC HITO 5  ", 14, false);
        ssd1306_display_text(&s_oled_dev, 2, "Iniciando...", 12, false);
        ESP_LOGI(TAG, "OLED inicializada con exito.");
    } else {
        ESP_LOGW(TAG, "No se detecto pantalla OLED en bus I2C.");
        s_oled_ready = false;
    }

    return ESP_OK;
}

void actuators_update_oled(float lux, uint32_t period_s, bool tb_connected, const char *last_rpc_method)
{
    // Actualizar 7 segmentos con la cadencia activa (0 a 9)
    int light_level = (int)(lux / 100.0f);
    if (light_level < 0) light_level = 0;
    if (light_level > 9) light_level = 9;
    
    actuators_set_7seg_digit(light_level);

    if (!s_oled_ready) {
        return;
    }

    char line_buf[20];
    ssd1306_clear_screen(&s_oled_dev, false);

    ssd1306_display_text(&s_oled_dev, 0, "SBC26T06 - HITO5", 16, true);

    snprintf(line_buf, sizeof(line_buf), "Luz: %.1f lux", lux);
    ssd1306_display_text(&s_oled_dev, 2, line_buf, strlen(line_buf), false);

    snprintf(line_buf, sizeof(line_buf), "Cadencia: %lu s", (unsigned long)period_s);
    ssd1306_display_text(&s_oled_dev, 4, line_buf, strlen(line_buf), false);

    if (last_rpc_method != NULL && strlen(last_rpc_method) > 0) {
        snprintf(line_buf, sizeof(line_buf), "RPC: %s", last_rpc_method);
    } else {
        snprintf(line_buf, sizeof(line_buf), "TB: %s", tb_connected ? "CONECTADO" : "CONECTANDO...");
    }
    ssd1306_display_text(&s_oled_dev, 6, line_buf, strlen(line_buf), false);
}
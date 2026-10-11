#ifndef WIFI_STA_H
#define WIFI_STA_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa la interfaz de red y el stack Wi-Fi en modo STA.
 *        Bloquea hasta obtener direccion IP por DHCP o superar los reintentos maximos.
 * 
 * @param ssid Nombre de la red Wi-Fi
 * @param pass Contraseña de acceso
 * @return esp_err_t ESP_OK si la conexion fue exitosa, ESP_FAIL en caso contrario.
 */
esp_err_t wifi_sta_init(const char *ssid, const char *pass);

/**
 * @brief Consulta el estado actual del enlace Wi-Fi.
 * 
 * @return true si el ESP32 cuenta con conexion activa e IP asignada.
 */
bool wifi_sta_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_STA_H
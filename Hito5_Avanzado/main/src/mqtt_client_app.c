#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"

#include "mqtt_client_app.h"
#include "app_config.h"

static const char *TAG = "TB_MQTT";

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_connected = false;
static rpc_cadence_callback_t s_rpc_callback = NULL;

/**
 * @brief Procesa el mensaje recibido en el topic v1/devices/me/rpc/request/{id}
 */
static void handle_rpc_request(const char *topic, int topic_len, const char *data, int data_len)
{
    // Extraer el identificador de la peticion desde el topic
    // Topic esperado: v1/devices/me/rpc/request/{requestId}
    const char *prefix = "v1/devices/me/rpc/request/";
    size_t prefix_len = strlen(prefix);

    char request_id[32] = {0};
    if (topic_len > prefix_len && strncmp(topic, prefix, prefix_len) == 0) {
        int id_len = topic_len - prefix_len;
        if (id_len < sizeof(request_id)) {
            memcpy(request_id, topic + prefix_len, id_len);
            request_id[id_len] = '\0';
        }
    }

    // Copiar payload a un buffer terminado en nulo para cJSON
    char *payload_str = malloc(data_len + 1);
    if (payload_str == NULL) {
        ESP_LOGE(TAG, "Memoria insuficiente para procesar la peticion RPC");
        return;
    }
    memcpy(payload_str, data, data_len);
    payload_str[data_len] = '\0';

    ESP_LOGI(TAG, "Comando RPC entrante [ID=%s]: %s", request_id, payload_str);

    cJSON *root = cJSON_Parse(payload_str);
    free(payload_str);

    if (root == NULL) {
        ESP_LOGE(TAG, "El payload RPC no contiene un JSON valido");
        return;
    }

    cJSON *method_item = cJSON_GetObjectItem(root, "method");
    cJSON *params_item = cJSON_GetObjectItem(root, "params");

    if (method_item != NULL && cJSON_IsString(method_item)) {
        const char *method = method_item->valuestring;
        ESP_LOGI(TAG, "Metodo solicitado: %s", method);

        // Control bidireccional: modificacion dinamica de cadencia
        if (strcmp(method, "setCadence") == 0 || strcmp(method, "setPeriod") == 0) {
            uint32_t new_period = DEFAULT_SAMPLE_PERIOD_S;

            if (params_item != NULL) {
                if (cJSON_IsNumber(params_item)) {
                    new_period = (uint32_t)params_item->valueint;
                } else if (cJSON_IsString(params_item)) {
                    new_period = (uint32_t)atoi(params_item->valuestring);
                }
            }

            // Aplicar limites de seguridad
            if (new_period < MIN_SAMPLE_PERIOD_S) new_period = MIN_SAMPLE_PERIOD_S;
            if (new_period > MAX_SAMPLE_PERIOD_S) new_period = MAX_SAMPLE_PERIOD_S;

            ESP_LOGI(TAG, "Nueva cadencia aplicada: %lu segundos", (unsigned long)new_period);

            if (s_rpc_callback != NULL) {
                s_rpc_callback(new_period, method);
            }

            // Responder a ThingsBoard para confirmar la ejecucion del comando
            if (strlen(request_id) > 0) {
                char response_topic[64];
                snprintf(response_topic, sizeof(response_topic), "%s%s", TB_RPC_RESPONSE_PREFIX, request_id);

                char response_payload[64];
                snprintf(response_payload, sizeof(response_payload), "{\"period\":%lu,\"status\":\"OK\"}", (unsigned long)new_period);

                esp_mqtt_client_publish(s_mqtt_client, response_topic, response_payload, 0, 1, 0);
                ESP_LOGI(TAG, "Respuesta RPC enviada a %s: %s", response_topic, response_payload);
            }
        } else {
            ESP_LOGW(TAG, "Metodo RPC desconocido recibido: %s", method);
        }
    }

    cJSON_Delete(root);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Conectado al broker de ThingsBoard");
        s_connected = true;

        // Suscribirse al topic de comandos RPC
        esp_mqtt_client_subscribe(s_mqtt_client, TB_RPC_REQUEST_TOPIC, 1);
        ESP_LOGI(TAG, "Suscrito al topic de comandos RPC: %s", TB_RPC_REQUEST_TOPIC);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Desconectado del broker de ThingsBoard");
        s_connected = false;
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGD(TAG, "Mensaje MQTT recibido en topic de longitud %d", event->topic_len);
        handle_rpc_request(event->topic, event->topic_len, event->data, event->data_len);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "Error en el cliente MQTT");
        break;

    default:
        break;
    }
}

esp_err_t mqtt_client_app_start(rpc_cadence_callback_t on_cadence)
{
    s_rpc_callback = on_cadence;

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = TB_BROKER_URI,
        .credentials.username = TB_DEVICE_TOKEN,
    };

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "Error al inicializar el cliente MQTT");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_mqtt_client));

    ESP_LOGI(TAG, "Cliente MQTT arrancado hacia %s", TB_BROKER_URI);
    return ESP_OK;
}

esp_err_t mqtt_client_app_publish_telemetry(const char *json_payload)
{
    if (s_mqtt_client == NULL || !s_connected) {
        return ESP_ERR_INVALID_STATE;
    }

    int msg_id = esp_mqtt_client_publish(s_mqtt_client, TB_TELEMETRY_TOPIC, json_payload, 0, 1, 0);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Fallo al publicar telemetria");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Telemetria publicada: %s", json_payload);
    return ESP_OK;
}

bool mqtt_client_app_is_connected(void)
{
    return s_connected;
}
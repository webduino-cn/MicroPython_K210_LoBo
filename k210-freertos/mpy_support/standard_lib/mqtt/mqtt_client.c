/*
 * This file is part of the MicroPython K210 project, https://github.com/loboris/MicroPython_K210_LoBo
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 LoBo (https://github.com/loboris)
 *
 * Slightly modified mqtt library from https://github.com/espressif/esp-mqtt
 *
 * Copyright (c) 2018 tuanpm (https://github.com/tuanpmt/espmqtt)
 * Copyright (c) 2018 LoBo (https://github.com/loboris)
*/

#include "platform_k210.h"

#ifdef MICROPY_PY_USE_MQTT

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mqtt_client.h"
#include "transport_tcp.h"
#include "transport_ssl.h"
#include "transport_ws.h"

/* using uri parser */
#include "http_parser.h"

typedef struct _task_params_t {
    void *mqtt_obj;
    void *thread_handle;
} task_params_t;

static task_params_t task_params;

const char *MQTT_TAG = "MQTT_CLIENT";

const static int STOPPED_BIT = 1;

static int esp_mqtt_dispatch_event(esp_mqtt_client_handle_t client);
static int esp_mqtt_set_config(esp_mqtt_client_handle_t client, const esp_mqtt_client_config_t *config);
static int esp_mqtt_destroy_config(esp_mqtt_client_handle_t client);
static int esp_mqtt_connect(esp_mqtt_client_handle_t client, int timeout_ms);
static int esp_mqtt_abort_connection(esp_mqtt_client_handle_t client);
static int esp_mqtt_client_ping(esp_mqtt_client_handle_t client);
static char *create_string(const char *ptr, int len);

static int esp_mqtt_set_config(esp_mqtt_client_handle_t client, const esp_mqtt_client_config_t *config)
{
    //Copy user configurations to client context
    int err = 0;
    mqtt_config_storage_t *cfg = pvPortMalloc(sizeof(mqtt_config_storage_t));
    K210_MEM_CHECK(MQTT_TAG, cfg, return K210_ERR_NO_MEM);
    memset(cfg, 0, sizeof(mqtt_config_storage_t));

    client->config = cfg;

    cfg->task_prio = config->task_prio;
    if (cfg->task_prio <= 0) {
        cfg->task_prio = MQTT_TASK_PRIORITY;
    }

    cfg->task_stack = config->task_stack;
    if (cfg->task_stack == 0) {
        cfg->task_stack = MQTT_TASK_STACK;
    }
    err = K210_ERR_NO_MEM;
    if (config->host[0]) {
        cfg->host = mqttstrdup(config->host);
        K210_MEM_CHECK(MQTT_TAG, cfg->host, goto _mqtt_set_config_failed);
    }
    cfg->port = config->port;

    if (config->username[0]) {
        client->connect_info.username = mqttstrdup(config->username);
        K210_MEM_CHECK(MQTT_TAG, client->connect_info.username, goto _mqtt_set_config_failed);
    }

    if (config->password[0]) {
        client->connect_info.password = mqttstrdup(config->password);
        K210_MEM_CHECK(MQTT_TAG, client->connect_info.password, goto _mqtt_set_config_failed);
    }

    if (config->client_id[0]) {
        client->connect_info.client_id = mqttstrdup(config->client_id);
    } else {
        client->connect_info.client_id = platform_create_id_string();
    }
    K210_MEM_CHECK(MQTT_TAG, client->connect_info.client_id, goto _mqtt_set_config_failed);
    if (transport_debug) LOGD(MQTT_TAG, "MQTT client_id=%s", client->connect_info.client_id);

    if (config->uri[0]) {
        cfg->uri = mqttstrdup(config->uri);
        K210_MEM_CHECK(MQTT_TAG, cfg->uri, goto _mqtt_set_config_failed);
    }

    if (config->lwt_topic[0]) {
        client->connect_info.will_topic = mqttstrdup(config->lwt_topic);
        K210_MEM_CHECK(MQTT_TAG, client->connect_info.will_topic, goto _mqtt_set_config_failed);
    }

    if (config->lwt_msg_len) {
        client->connect_info.will_message = pvPortMalloc(config->lwt_msg_len);
        K210_MEM_CHECK(MQTT_TAG, client->connect_info.will_message, goto _mqtt_set_config_failed);
        memcpy(client->connect_info.will_message, config->lwt_msg, config->lwt_msg_len);
        client->connect_info.will_length = config->lwt_msg_len;
    } else if (config->lwt_msg[0]) {
        client->connect_info.will_message = mqttstrdup(config->lwt_msg);
        K210_MEM_CHECK(MQTT_TAG, client->connect_info.will_message, goto _mqtt_set_config_failed);
        client->connect_info.will_length = strlen(config->lwt_msg);
    }

    client->connect_info.will_qos = config->lwt_qos;
    client->connect_info.will_retain = config->lwt_retain;

    client->connect_info.clean_session = 1;
    if (config->disable_clean_session) {
        client->connect_info.clean_session = false;
    }
    client->connect_info.keepalive = config->keepalive;
    if (client->connect_info.keepalive == 0) {
        client->connect_info.keepalive = MQTT_KEEPALIVE_TICK;
    }
    cfg->network_timeout_ms = MQTT_NETWORK_TIMEOUT_MS;
    cfg->user_context = config->user_context;
    cfg->event_handle = config->event_handle;
    cfg->auto_reconnect = true;
    if (config->disable_auto_reconnect) {
        cfg->auto_reconnect = false;
    }

    return 0;

_mqtt_set_config_failed:
    esp_mqtt_destroy_config(client);
    return err;
}

static int esp_mqtt_destroy_config(esp_mqtt_client_handle_t client)
{
    mqtt_config_storage_t *cfg = client->config;
    if (cfg->host) vPortFree(cfg->host);
    if (cfg->uri) vPortFree(cfg->uri);
    if (cfg->path) vPortFree(cfg->path);
    if (cfg->scheme) vPortFree(cfg->scheme);
    if (client->connect_info.will_topic) vPortFree(client->connect_info.will_topic);
    if (client->connect_info.will_message) vPortFree(client->connect_info.will_message);
    if (client->connect_info.client_id) vPortFree(client->connect_info.client_id);
    if (client->connect_info.username) vPortFree(client->connect_info.username);
    if (client->connect_info.password)vPortFree(client->connect_info.password);
    vPortFree(client->config);
    return 0;
}

static int esp_mqtt_connect(esp_mqtt_client_handle_t client, int timeout_ms)
{
    int write_len, read_len, connect_rsp_code;
    client->wait_for_ping_resp = false;
    mqtt_msg_init(&client->mqtt_state.mqtt_connection,
                  client->mqtt_state.out_buffer,
                  client->mqtt_state.out_buffer_length);
    client->mqtt_state.outbound_message = mqtt_msg_connect(&client->mqtt_state.mqtt_connection,
                                          client->mqtt_state.connect_info);
    client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    client->mqtt_state.pending_msg_id = mqtt_get_id(client->mqtt_state.outbound_message->data,
                                        client->mqtt_state.outbound_message->length);
    if (transport_debug) LOGI(MQTT_TAG, "Sending MQTT CONNECT message, type: %d, id: %04X",
             client->mqtt_state.pending_msg_type,
             client->mqtt_state.pending_msg_id);

    write_len = transport_write(client->transport,
                                (char *)client->mqtt_state.outbound_message->data,
                                client->mqtt_state.outbound_message->length,
                                client->config->network_timeout_ms);
    if (write_len < 0) {
        if (transport_debug) LOGE(MQTT_TAG, "Writing failed, errno= %d", errno);
        return -1;
    }
    else {
        if (transport_debug) LOGD(MQTT_TAG, "OK, written %d bytes, waiting response", write_len);
    }
    read_len = transport_read(client->transport,
                              (char *)client->mqtt_state.in_buffer,
                              client->mqtt_state.outbound_message->length,
                              client->config->network_timeout_ms);
    if (read_len < 0) {
        if (transport_debug) LOGE(MQTT_TAG, "Error network response");
        return -1;
    }
    else {
        if (transport_debug) LOGD(MQTT_TAG, "OK, read %d bytes", read_len);
    }

    if (mqtt_get_type(client->mqtt_state.in_buffer) != MQTT_MSG_TYPE_CONNACK) {
        if (transport_debug) LOGE(MQTT_TAG, "Invalid MSG_TYPE response: %d, read_len: %d", mqtt_get_type(client->mqtt_state.in_buffer), read_len);
        return -1;
    }
    connect_rsp_code = mqtt_get_connect_return_code(client->mqtt_state.in_buffer);
    switch (connect_rsp_code) {
        case CONNECTION_ACCEPTED:
            if (transport_debug) LOGD(MQTT_TAG, "Connected");
            return 0;
        case CONNECTION_REFUSE_PROTOCOL:
            if (transport_debug) LOGW(MQTT_TAG, "Connection refused, bad protocol");
            return -1;
        case CONNECTION_REFUSE_SERVER_UNAVAILABLE:
            if (transport_debug) LOGW(MQTT_TAG, "Connection refused, server unavailable");
            return -1;
        case CONNECTION_REFUSE_BAD_USERNAME:
            if (transport_debug) LOGW(MQTT_TAG, "Connection refused, bad username or password");
            return -1;
        case CONNECTION_REFUSE_NOT_AUTHORIZED:
            if (transport_debug) LOGW(MQTT_TAG, "Connection refused, not authorized");
            return -1;
        default:
            if (transport_debug) LOGW(MQTT_TAG, "Connection refused, Unknow reason");
            return -1;
    }
    return 0;
}

static int esp_mqtt_abort_connection(esp_mqtt_client_handle_t client)
{
    transport_close(client->transport);
    client->wait_timeout_ms = MQTT_RECONNECT_TIMEOUT_MS;
    client->reconnect_tick = platform_tick_get_ms();
    client->state = MQTT_STATE_WAIT_TIMEOUT;
    if (transport_debug) LOGI(MQTT_TAG, "Reconnect after %d ms", client->wait_timeout_ms);
    client->event.event_id = MQTT_EVENT_DISCONNECTED;
    client->wait_for_ping_resp = false;
    esp_mqtt_dispatch_event(client);
    return 0;
}

esp_mqtt_client_handle_t esp_mqtt_client_init(const esp_mqtt_client_config_t *config)
{
    esp_mqtt_client_handle_t client = pvPortMalloc(sizeof(struct esp_mqtt_client));
    K210_MEM_CHECK(MQTT_TAG, client, return NULL);
    memset(client, 0, sizeof(struct esp_mqtt_client));

    if (esp_mqtt_set_config(client, config) != 0) {
        if (transport_debug) LOGE(MQTT_TAG, "Setting configuration failed");
        return NULL;
    }

    client->transport_list = transport_list_init();
    K210_MEM_CHECK(MQTT_TAG, client->transport_list, goto _mqtt_init_failed);

    transport_handle_t tcp = transport_tcp_init();
    if (tcp) {
        transport_set_default_port(tcp, MQTT_TCP_DEFAULT_PORT);
        transport_list_add(client->transport_list, tcp, "mqtt");
        if (config->transport == MQTT_TRANSPORT_OVER_TCP) {
            client->config->scheme = create_string("mqtt", 4);
            K210_MEM_CHECK(MQTT_TAG, client->config->scheme, goto _mqtt_init_failed);
        }
    }
    else if (config->transport == MQTT_TRANSPORT_OVER_TCP) goto _mqtt_init_failed;

    transport_handle_t ws = transport_ws_init(tcp);
    if (ws) {
        transport_set_default_port(ws, MQTT_WS_DEFAULT_PORT);
        transport_list_add(client->transport_list, ws, "ws");
        if (config->transport == MQTT_TRANSPORT_OVER_WS) {
            client->config->scheme = create_string("ws", 2);
            K210_MEM_CHECK(MQTT_TAG, client->config->scheme, goto _mqtt_init_failed);
        }
    }
    else if (config->transport == MQTT_TRANSPORT_OVER_WS) goto _mqtt_init_failed;

    if ((net_active_interfaces & ACTIVE_INTERFACE_WIFI)) {
        transport_handle_t ssl = transport_ssl_init();
        if (ssl) {
            transport_set_default_port(ssl, MQTT_SSL_DEFAULT_PORT);
            if (config->cert_pem) {
                transport_ssl_set_cert_data(ssl, config->cert_pem, strlen(config->cert_pem));
            }
            if (config->client_cert_pem) {
                transport_ssl_set_client_cert_data(ssl, config->client_cert_pem, strlen(config->client_cert_pem));
            }
            if (config->client_key_pem) {
                transport_ssl_set_client_key_data(ssl, config->client_key_pem, strlen(config->client_key_pem));
            }
            transport_list_add(client->transport_list, ssl, "mqtts");
            if (config->transport == MQTT_TRANSPORT_OVER_SSL) {
                client->config->scheme = create_string("mqtts", 5);
                K210_MEM_CHECK(MQTT_TAG, client->config->scheme, goto _mqtt_init_failed);
            }
        }
        else if (config->transport == MQTT_TRANSPORT_OVER_SSL) goto _mqtt_init_failed;

        transport_handle_t wss = transport_ws_init(ssl);
        if (wss) {
            transport_set_default_port(wss, MQTT_WSS_DEFAULT_PORT);
            transport_list_add(client->transport_list, wss, "wss");
            if (config->transport == MQTT_TRANSPORT_OVER_WSS) {
                client->config->scheme = create_string("wss", 3);
                K210_MEM_CHECK(MQTT_TAG, client->config->scheme, goto _mqtt_init_failed);
            }
        }
        else if (config->transport == MQTT_TRANSPORT_OVER_WSS) goto _mqtt_init_failed;
    }
    else if ((config->transport == MQTT_TRANSPORT_OVER_WSS) || (config->transport == MQTT_TRANSPORT_OVER_WSS)) goto _mqtt_init_failed;

    if (client->config->uri) {
        if (esp_mqtt_client_set_uri(client, client->config->uri) != 0) {
            goto _mqtt_init_failed;
        }
    }

    if (client->config->scheme == NULL) {
        client->config->scheme = create_string("mqtt", 4);
        K210_MEM_CHECK(MQTT_TAG, client->config->scheme, goto _mqtt_init_failed);
    }

    client->keepalive_tick = platform_tick_get_ms();
    client->reconnect_tick = platform_tick_get_ms();
    client->wait_for_ping_resp = false;
    int buffer_size = config->buffer_size;
    if (buffer_size <= 0) {
        buffer_size = MQTT_BUFFER_SIZE_BYTE;
    }

    client->mqtt_state.in_buffer = (uint8_t *)pvPortMalloc(buffer_size);
    K210_MEM_CHECK(MQTT_TAG, client->mqtt_state.in_buffer, goto _mqtt_init_failed);
    client->mqtt_state.in_buffer_length = buffer_size;
    client->mqtt_state.out_buffer = (uint8_t *)pvPortMalloc(buffer_size);
    K210_MEM_CHECK(MQTT_TAG, client->mqtt_state.out_buffer, goto _mqtt_init_failed);

    client->mqtt_state.out_buffer_length = buffer_size;
    client->mqtt_state.connect_info = &client->connect_info;
    client->outbox = outbox_init();
    K210_MEM_CHECK(MQTT_TAG, client->outbox, goto _mqtt_init_failed);
    client->status_bits = xEventGroupCreate();
    K210_MEM_CHECK(MQTT_TAG, client->status_bits, goto _mqtt_init_failed);
    return client;

_mqtt_init_failed:
    esp_mqtt_client_destroy(client);
    return NULL;
}

int esp_mqtt_client_destroy(esp_mqtt_client_handle_t client)
{
    esp_mqtt_client_stop(client);
    esp_mqtt_destroy_config(client);
    transport_list_destroy(client->transport_list);
    outbox_destroy(client->outbox);
    if (client->status_bits) vEventGroupDelete(client->status_bits);
    if (client->mqtt_state.in_buffer) vPortFree(client->mqtt_state.in_buffer);
    if (client->mqtt_state.out_buffer) vPortFree(client->mqtt_state.out_buffer);
    vPortFree(client);
    return 0;
}

static char *create_string(const char *ptr, int len)
{
    char *ret;
    if (len <= 0) {
        return NULL;
    }
    ret = pvPortMalloc(len + 1);
    K210_MEM_CHECK(MQTT_TAG, ret, return NULL);
    memset(ret, 0, len+1);
    memcpy(ret, ptr, len);
    return ret;
}

int esp_mqtt_client_set_uri(esp_mqtt_client_handle_t client, const char *uri)
{
    struct http_parser_url puri;
    http_parser_url_init(&puri);
    int parser_status = http_parser_parse_url(uri, strlen(uri), 0, &puri);
    if (parser_status != 0) {
        if (transport_debug) LOGE(MQTT_TAG, "Error parsing uri = %s", uri);
        return -1;
    }

    if (client->config->scheme == NULL) {
        client->config->scheme = create_string(uri + puri.field_data[UF_SCHEMA].off, puri.field_data[UF_SCHEMA].len);
    }

    if (client->config->host == NULL) {
        client->config->host = create_string(uri + puri.field_data[UF_HOST].off, puri.field_data[UF_HOST].len);
    }

    if (client->config->path == NULL) {
        client->config->path = create_string(uri + puri.field_data[UF_PATH].off, puri.field_data[UF_PATH].len);
    }
    if (client->config->path) {
        transport_handle_t trans = transport_list_get_transport(client->transport_list, "ws");
        if (trans) {
            transport_ws_set_path(trans, client->config->path);
        }
        trans = transport_list_get_transport(client->transport_list, "wss");
        if (trans) {
            transport_ws_set_path(trans, client->config->path);
        }
    }

    if (puri.field_data[UF_PORT].len) {
        client->config->port = strtol((const char*)(uri + puri.field_data[UF_PORT].off), NULL, 10);
    }

    char *user_info = create_string(uri + puri.field_data[UF_USERINFO].off, puri.field_data[UF_USERINFO].len);
    if (user_info) {
        char *pass = strchr(user_info, ':');
        if (pass) {
            pass[0] = 0; //terminal username
            pass ++;
            client->connect_info.password = mqttstrdup(pass);
        }
        client->connect_info.username = mqttstrdup(user_info);

        vPortFree(user_info);
    }

    return 0;
}

static int mqtt_write_data(esp_mqtt_client_handle_t client)
{
    int write_len = transport_write(client->transport,
                                    (char *)client->mqtt_state.outbound_message->data,
                                    client->mqtt_state.outbound_message->length,
                                    client->config->network_timeout_ms);
    // client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    if (write_len <= 0) {
        if (transport_debug) LOGE(MQTT_TAG, "Error write data or timeout, written len = %d", write_len);
        return -1;
    }
    /* we've just sent a mqtt control packet, update keepalive counter
     * [MQTT-3.1.2-23]
     */
    client->keepalive_tick = platform_tick_get_ms();
    return 0;
}

static int esp_mqtt_dispatch_event(esp_mqtt_client_handle_t client)
{
    client->event.msg_id = mqtt_get_id(client->mqtt_state.in_buffer, client->mqtt_state.in_buffer_length);
    client->event.user_context = client->config->user_context;
    client->event.client = client;

    if (client->config->event_handle) {
        return client->config->event_handle(&client->event);
    }
    return -1;
}



static void deliver_publish(esp_mqtt_client_handle_t client, uint8_t *message, int length)
{
    const char *mqtt_topic, *mqtt_data;
    uint32_t mqtt_topic_length, mqtt_data_length;
    uint32_t mqtt_len, mqtt_offset = 0, total_mqtt_len = 0;
    int len_read;

    do
    {
        mqtt_topic_length = length;
        mqtt_topic = mqtt_get_publish_topic(message, &mqtt_topic_length);
        mqtt_data_length = length;
        mqtt_data = mqtt_get_publish_data(message, &mqtt_data_length);

        if (total_mqtt_len == 0) {
            mqtt_topic_length = length;
            mqtt_topic = mqtt_get_publish_topic(message, &mqtt_topic_length);
            mqtt_data_length = length;
            mqtt_data = mqtt_get_publish_data(message, &mqtt_data_length);
            total_mqtt_len = client->mqtt_state.message_length - client->mqtt_state.message_length_read + mqtt_data_length;
            mqtt_len = mqtt_data_length;
        } else {
            mqtt_len = len_read;
            mqtt_data = (const char*)client->mqtt_state.in_buffer;
        }

        if (transport_debug) LOGD(MQTT_TAG, "Get data len= %d, topic len=%d", mqtt_data_length, mqtt_topic_length);
        client->event.event_id = MQTT_EVENT_DATA;
        client->event.data = (char *)mqtt_data;
        client->event.data_len = mqtt_len;
        client->event.total_data_len = total_mqtt_len;
        client->event.current_data_offset = mqtt_offset;
        client->event.topic = (char *)mqtt_topic;
        client->event.topic_len = mqtt_topic_length;
        esp_mqtt_dispatch_event(client);

        mqtt_offset += mqtt_len;
        if (client->mqtt_state.message_length_read >= client->mqtt_state.message_length) {
            break;
        }

        /*len_read = transport_read(client->transport,
                                      (char *)client->mqtt_state.in_buffer,
                                      client->mqtt_state.in_buffer_length,
                                      client->config->network_timeout_ms);*/
        len_read = transport_read(client->transport,
                                  (char *)client->mqtt_state.in_buffer,
                                  client->mqtt_state.message_length - client->mqtt_state.message_length_read > client->mqtt_state.in_buffer_length ?
                                  client->mqtt_state.in_buffer_length : client->mqtt_state.message_length - client->mqtt_state.message_length_read,
                                  client->config->network_timeout_ms);
        if (len_read <= 0) {
            if (transport_debug) LOGE(MQTT_TAG, "Read error or timeout: %d", errno);
            break;
        }
        client->mqtt_state.message_length_read += len_read;
    } while (1);


}

static bool is_valid_mqtt_msg(esp_mqtt_client_handle_t client, int msg_type, int msg_id)
{
    if (transport_debug) LOGD(MQTT_TAG, "pending_id=%d, pending_msg_count = %d", client->mqtt_state.pending_msg_id, client->mqtt_state.pending_msg_count);
    if (client->mqtt_state.pending_msg_count == 0) {
        return false;
    }
    if (outbox_delete(client->outbox, msg_id, msg_type) == 0) {
        client->mqtt_state.pending_msg_count --;
        return true;
    }
    if (client->mqtt_state.pending_msg_type == msg_type && client->mqtt_state.pending_msg_id == msg_id) {
        client->mqtt_state.pending_msg_count --;
        return true;
    }

    return false;
}

static void mqtt_enqueue(esp_mqtt_client_handle_t client)
{
    if (transport_debug) LOGD(MQTT_TAG, "mqtt_enqueue id: %d, type=%d successful",
             client->mqtt_state.pending_msg_id, client->mqtt_state.pending_msg_type);
    //lock mutex
    if (client->mqtt_state.pending_msg_count > 0) {
        //Copy to queue buffer
        outbox_enqueue(client->outbox,
                       client->mqtt_state.outbound_message->data,
                       client->mqtt_state.outbound_message->length,
                       client->mqtt_state.pending_msg_id,
                       client->mqtt_state.pending_msg_type,
                       platform_tick_get_ms());
    }
    //unlock
}

static int mqtt_process_receive(esp_mqtt_client_handle_t client)
{
    int read_len;
    uint8_t msg_type;
    uint8_t msg_qos;
    uint16_t msg_id;

    read_len = transport_read(client->transport, (char *)client->mqtt_state.in_buffer, client->mqtt_state.in_buffer_length, 1000);

    if (read_len < 0) {
        if (transport_debug) LOGE(MQTT_TAG, "Read error or end of stream");
        return -1;
    }

    if (read_len == 0) {
        return 0;
    }

    msg_type = mqtt_get_type(client->mqtt_state.in_buffer);
    msg_qos = mqtt_get_qos(client->mqtt_state.in_buffer);
    msg_id = mqtt_get_id(client->mqtt_state.in_buffer, client->mqtt_state.in_buffer_length);

    if (transport_debug) LOGD(MQTT_TAG, "msg_type=%d, msg_id=%d", msg_type, msg_id);
    switch (msg_type)
    {
        case MQTT_MSG_TYPE_SUBACK:
            if (is_valid_mqtt_msg(client, MQTT_MSG_TYPE_SUBSCRIBE, msg_id)) {
                if (transport_debug) LOGD(MQTT_TAG, "Subscribe successful");
                client->event.event_id = MQTT_EVENT_SUBSCRIBED;
                client->event.type = MQTT_MSG_TYPE_SUBSCRIBE;
                esp_mqtt_dispatch_event(client);
            }
            break;
        case MQTT_MSG_TYPE_UNSUBACK:
            if (is_valid_mqtt_msg(client, MQTT_MSG_TYPE_UNSUBSCRIBE, msg_id)) {
                if (transport_debug) LOGD(MQTT_TAG, "UnSubscribe successful");
                client->event.event_id = MQTT_EVENT_UNSUBSCRIBED;
                client->event.type = MQTT_MSG_TYPE_UNSUBSCRIBE;
                esp_mqtt_dispatch_event(client);
            }
            break;
        case MQTT_MSG_TYPE_PUBLISH:
            if (msg_qos == 1) {
                client->mqtt_state.outbound_message = mqtt_msg_puback(&client->mqtt_state.mqtt_connection, msg_id);
            }
            else if (msg_qos == 2) {
                client->mqtt_state.outbound_message = mqtt_msg_pubrec(&client->mqtt_state.mqtt_connection, msg_id);
            }

            if (msg_qos == 1 || msg_qos == 2) {
                if (transport_debug) LOGD(MQTT_TAG, "Queue response QoS: %d", msg_qos);

                if (mqtt_write_data(client) != 0) {
                    if (transport_debug) LOGE(MQTT_TAG, "Error write qos msg repsonse, qos = %d", msg_qos);
                    // TODO: Should reconnect?
                    // return -1;
                }
            }
            client->mqtt_state.message_length_read = read_len;
            client->mqtt_state.message_length = mqtt_get_total_length(client->mqtt_state.in_buffer, client->mqtt_state.message_length_read);
            if (transport_debug) LOGI(MQTT_TAG, "deliver_publish, message_length_read=%d, message_length=%d", read_len, client->mqtt_state.message_length);

            deliver_publish(client, client->mqtt_state.in_buffer, client->mqtt_state.message_length_read);
            break;
        case MQTT_MSG_TYPE_PUBACK:
            if (is_valid_mqtt_msg(client, MQTT_MSG_TYPE_PUBLISH, msg_id)) {
                if (transport_debug) LOGD(MQTT_TAG, "received MQTT_MSG_TYPE_PUBACK, finish QoS1 publish");
                client->event.event_id = MQTT_EVENT_PUBLISHED;
                client->event.type = MQTT_MSG_TYPE_PUBACK;
                esp_mqtt_dispatch_event(client);
            }

            break;
        case MQTT_MSG_TYPE_PUBREC:
            if (transport_debug) LOGD(MQTT_TAG, "received MQTT_MSG_TYPE_PUBREC");
            client->mqtt_state.outbound_message = mqtt_msg_pubrel(&client->mqtt_state.mqtt_connection, msg_id);
            mqtt_write_data(client);
            break;
        case MQTT_MSG_TYPE_PUBREL:
            if (transport_debug) LOGD(MQTT_TAG, "received MQTT_MSG_TYPE_PUBREL");
            client->mqtt_state.outbound_message = mqtt_msg_pubcomp(&client->mqtt_state.mqtt_connection, msg_id);
            mqtt_write_data(client);

            break;
        case MQTT_MSG_TYPE_PUBCOMP:
            if (transport_debug) LOGD(MQTT_TAG, "received MQTT_MSG_TYPE_PUBCOMP");
            if (is_valid_mqtt_msg(client, MQTT_MSG_TYPE_PUBLISH, msg_id)) {
                if (transport_debug) LOGD(MQTT_TAG, "Receive MQTT_MSG_TYPE_PUBCOMP, finish QoS2 publish");
                client->event.event_id = MQTT_EVENT_PUBLISHED;
                client->event.type = MQTT_MSG_TYPE_PUBCOMP;
                esp_mqtt_dispatch_event(client);
            }
            break;
        case MQTT_MSG_TYPE_PINGREQ:
            client->mqtt_state.outbound_message = mqtt_msg_pingresp(&client->mqtt_state.mqtt_connection);
            mqtt_write_data(client);
            break;
        case MQTT_MSG_TYPE_PINGRESP:
            if (transport_debug) LOGD(MQTT_TAG, "MQTT_MSG_TYPE_PINGRESP");
            client->wait_for_ping_resp = false;
            // Ignore
            break;
    }

    return 0;
}

//===========================================
static void esp_mqtt_task(void *pvParameters)
{
    task_params_t *task_params = (task_params_t *)pvParameters;
    // if the task uses some MicroPython functions, we have to save
    // MicroPython state in local storage pointers
    vTaskSetThreadLocalStoragePointer(NULL, THREAD_LSP_STATE, pvTaskGetThreadLocalStoragePointer((TaskHandle_t)task_params->thread_handle, THREAD_LSP_STATE));
    vTaskSetThreadLocalStoragePointer(NULL, THREAD_LSP_ARGS, pvTaskGetThreadLocalStoragePointer((TaskHandle_t)task_params->thread_handle, THREAD_LSP_ARGS));

    if (transport_debug) LOGD(MQTT_TAG, "Task started.");

    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)task_params->mqtt_obj;
    if (!client) goto exit;
    client->run = true;

    //get transport by scheme
    client->transport = transport_list_get_transport(client->transport_list, client->config->scheme);

    if (client->transport == NULL) {
        if (transport_debug) LOGE(MQTT_TAG, "There are no transports valid, stop mqtt client, config scheme = %s", client->config->scheme);
        client->run = false;
    }
    //default port
    if (client->config->port == 0) {
        client->config->port = transport_get_default_port(client->transport);
    }

    client->state = MQTT_STATE_INIT;
    xEventGroupClearBits(client->status_bits, STOPPED_BIT);
    while (client->run) {

        switch ((int)client->state) {
            case MQTT_STATE_INIT:
                if (client->transport == NULL) {
                    if (transport_debug) LOGE(MQTT_TAG, "There are no transport");
                    client->run = false;
                }

                if (transport_debug) LOGD(MQTT_TAG, "Connecting to server, timeout %d ms...", client->config->network_timeout_ms);
                if (transport_connect(client->transport,
                                      client->config->host,
                                      client->config->port,
                                      client->config->network_timeout_ms) < 0) {
                    if (transport_debug) LOGE(MQTT_TAG, "Transport connect error");
                    esp_mqtt_abort_connection(client);
                    break;
                }
                if (transport_debug) LOGD(MQTT_TAG, "Transport connected to %s://%s:%d", client->config->scheme, client->config->host, client->config->port);
                if (esp_mqtt_connect(client, client->config->network_timeout_ms) != 0) {
                    if (transport_debug) LOGI(MQTT_TAG, "MQTT Connect error");
                    esp_mqtt_abort_connection(client);
                    break;
                }
                client->event.event_id = MQTT_EVENT_CONNECTED;
                client->state = MQTT_STATE_CONNECTED;
                esp_mqtt_dispatch_event(client);

                break;
            case MQTT_STATE_CONNECTED:
                // receive and process data
                if (mqtt_process_receive(client) == -1) {
                    esp_mqtt_abort_connection(client);
                    break;
                }

                if (platform_tick_get_ms() - client->keepalive_tick > client->connect_info.keepalive * 1000 / 2) {
                    //No ping resp from last ping => Disconnected
                    if(client->wait_for_ping_resp){
                        if (transport_debug) LOGE(MQTT_TAG, "No PING_RESP, disconnected");
                        esp_mqtt_abort_connection(client);
                        client->wait_for_ping_resp = false;
                        break;
                    }
                    if (esp_mqtt_client_ping(client) == -1) {
                        if (transport_debug) LOGE(MQTT_TAG, "Can't send ping, disconnected");
                        esp_mqtt_abort_connection(client);
                        break;
                    }
                    else {
                        client->wait_for_ping_resp = true;
                    }
                    client->keepalive_tick = platform_tick_get_ms();
                }

                //Delete message after 30 seconds
                outbox_delete_expired(client->outbox, platform_tick_get_ms(), OUTBOX_EXPIRED_TIMEOUT_MS);
                //
                outbox_cleanup(client->outbox, OUTBOX_MAX_SIZE);
                break;
            case MQTT_STATE_WAIT_TIMEOUT:

                if (!client->config->auto_reconnect) {
                    client->run = false;
                    break;
                }
                if (platform_tick_get_ms() - client->reconnect_tick > client->wait_timeout_ms) {
                    client->state = MQTT_STATE_INIT;
                    client->reconnect_tick = platform_tick_get_ms();
                    if (transport_debug) LOGD(MQTT_TAG, "Reconnecting...");
                }
                vTaskDelay(client->wait_timeout_ms / 2 / portTICK_RATE_MS);
                break;
        }
    }
    transport_close(client->transport);
    if (client->status_bits) xEventGroupSetBits(client->status_bits, STOPPED_BIT);

exit:
    client->state = MQTT_STATE_UNKNOWN;
    if (transport_debug) LOGD(MQTT_TAG, "Task finished.");
    vTaskDelete(NULL);
}

int esp_mqtt_client_start(esp_mqtt_client_handle_t client)
{
    if (client->state >= MQTT_STATE_INIT) {
        if (transport_debug) LOGE(MQTT_TAG, "Client has started");
        return -1;
    }

    task_params.mqtt_obj = (void *)client;
    task_params.thread_handle = xTaskGetCurrentTaskHandle();
    BaseType_t tres = xTaskCreateAtProcessor(
            MAIN_TASK_PROC,             // processor
            esp_mqtt_task,              // function entry
            "mqtt_task",                // task name
            client->config->task_stack, // stack_deepth
            (void *)&task_params,       // function argument
            client->config->task_prio,  // task priority
            NULL);                      // task handle
    if (tres != pdTRUE) {
        if (transport_debug) LOGE(MQTT_TAG, "Error creating mqtt task");
        return -1;
    }
    return 0;
}


int esp_mqtt_client_stop(esp_mqtt_client_handle_t client)
{
    client->run = false;
    xEventGroupWaitBits(client->status_bits, STOPPED_BIT, false, true, 5000 / portTICK_RATE_MS);
    client->state = MQTT_STATE_UNKNOWN;
    return 0;
}

static int esp_mqtt_client_ping(esp_mqtt_client_handle_t client)
{
    client->mqtt_state.outbound_message = mqtt_msg_pingreq(&client->mqtt_state.mqtt_connection);

    if (mqtt_write_data(client) != 0) {
        if (transport_debug) LOGE(MQTT_TAG, "Error sending ping");
        return -1;
    }
    if (transport_debug) LOGD(MQTT_TAG, "Sent PING successful");
    return 0;
}

int esp_mqtt_client_subscribe(esp_mqtt_client_handle_t client, const char *topic, int qos)
{
    if (client->state != MQTT_STATE_CONNECTED) {
        if (transport_debug) LOGE(MQTT_TAG, "Client has not connected");
        return -1;
    }
    mqtt_enqueue(client); //move pending msg to outbox (if have)
    client->mqtt_state.outbound_message = mqtt_msg_subscribe(&client->mqtt_state.mqtt_connection,
                                          topic, qos,
                                          &client->mqtt_state.pending_msg_id);

    client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    client->mqtt_state.pending_msg_count ++;

    if (mqtt_write_data(client) != 0) {
        if (transport_debug) LOGE(MQTT_TAG, "Error to subscribe topic=%s, qos=%d", topic, qos);
        return -1;
    }

    if (transport_debug) LOGD(MQTT_TAG, "Sent subscribe topic=%s, id: %d, type=%d successful", topic, client->mqtt_state.pending_msg_id, client->mqtt_state.pending_msg_type);
    return client->mqtt_state.pending_msg_id;
}

int esp_mqtt_client_unsubscribe(esp_mqtt_client_handle_t client, const char *topic)
{
    if (client->state != MQTT_STATE_CONNECTED) {
        if (transport_debug) LOGE(MQTT_TAG, "Client has not connected");
        return -1;
    }
    mqtt_enqueue(client);
    client->mqtt_state.outbound_message = mqtt_msg_unsubscribe(&client->mqtt_state.mqtt_connection,
                                          topic,
                                          &client->mqtt_state.pending_msg_id);
    if (transport_debug) LOGD(MQTT_TAG, "unsubscribe, topic\"%s\", id: %d", topic, client->mqtt_state.pending_msg_id);

    client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    client->mqtt_state.pending_msg_count ++;

    if (mqtt_write_data(client) != 0) {
        if (transport_debug) LOGE(MQTT_TAG, "Error to unsubscribe topic=%s", topic);
        return -1;
    }

    if (transport_debug) LOGD(MQTT_TAG, "Sent Unsubscribe topic=%s, id: %d, successful", topic, client->mqtt_state.pending_msg_id);
    return client->mqtt_state.pending_msg_id;
}

int esp_mqtt_client_publish(esp_mqtt_client_handle_t client, const char *topic, const char *data, int len, int qos, int retain)
{
    uint16_t pending_msg_id = 0;
    if (client->state != MQTT_STATE_CONNECTED) {
        if (transport_debug) LOGE(MQTT_TAG, "Client has not connected");
        return -1;
    }
    if (len <= 0) {
        len = strlen(data);
    }
    // ToDo: is it needed here or later ?
    if (qos > 0) {
        mqtt_enqueue(client);
    }

    client->mqtt_state.outbound_message = mqtt_msg_publish(&client->mqtt_state.mqtt_connection,
                                          topic, data, len,
                                          qos, retain,
                                          &pending_msg_id);
    if (qos > 0) {
        client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
        client->mqtt_state.pending_msg_id = pending_msg_id;
        client->mqtt_state.pending_msg_count ++;
        mqtt_enqueue(client);
    }

    if (mqtt_write_data(client) != 0) {
        if (transport_debug) LOGE(MQTT_TAG, "Error publishing data to topic=%s, qos=%d", topic, qos);
        return -1;
    }
    return pending_msg_id;
}

#endif
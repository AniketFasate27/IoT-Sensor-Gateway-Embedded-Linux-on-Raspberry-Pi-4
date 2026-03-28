#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "MQTTClient.h"
#include <stdbool.h>

typedef struct {
    MQTTClient  handle;
    char        client_id[64];
    char        topic[128];
    bool        connected;
} mqtt_ctx_t;

int  mqtt_init(mqtt_ctx_t *ctx, const char *client_id);
int  mqtt_connect(mqtt_ctx_t *ctx);
int  mqtt_publish(mqtt_ctx_t *ctx, const char *payload);
void mqtt_disconnect(mqtt_ctx_t *ctx);
bool mqtt_is_connected(mqtt_ctx_t *ctx);

#endif

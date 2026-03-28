#include "mqtt_client.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

static void on_connection_lost(void *ctx, char *cause) {
    mqtt_ctx_t *m = (mqtt_ctx_t *)ctx;
    m->connected  = false;
    fprintf(stderr, "mqtt: connection lost: %s\n", cause ? cause : "unknown");
}

int mqtt_init(mqtt_ctx_t *ctx, const char *client_id) {
    char uri[256];
    snprintf(uri, sizeof(uri), "ssl://%s:%d", MQTT_HOST, MQTT_PORT);
    snprintf(ctx->client_id, sizeof(ctx->client_id), "%s", client_id);
    snprintf(ctx->topic, sizeof(ctx->topic), MQTT_TOPIC_FMT, client_id);
    ctx->connected = false;

    int rc = MQTTClient_create(&ctx->handle, uri, ctx->client_id,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "mqtt: create failed: %d\n", rc); return -1;
    }
    MQTTClient_setCallbacks(ctx->handle, ctx, on_connection_lost, NULL, NULL);
    return 0;
}

int mqtt_connect(mqtt_ctx_t *ctx) {
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    MQTTClient_SSLOptions      ssl = MQTTClient_SSLOptions_initializer;

    ssl.trustStore      = TLS_CA_CERT;
    ssl.keyStore        = TLS_CLIENT_CERT;
    ssl.privateKey      = TLS_PRIVATE_KEY;
    ssl.enableServerCertAuth = 1;

    opts.keepAliveInterval = MQTT_KEEPALIVE;
    opts.cleansession      = 1;
    opts.ssl               = &ssl;

    int rc = MQTTClient_connect(ctx->handle, &opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "mqtt: connect failed: %d\n", rc); return -1;
    }
    ctx->connected = true;
    printf("mqtt: connected to %s, topic: %s\n", MQTT_HOST, ctx->topic);
    return 0;
}

int mqtt_publish(mqtt_ctx_t *ctx, const char *payload) {
    MQTTClient_message msg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    msg.payload    = (void *)payload;
    msg.payloadlen = (int)strlen(payload);
    msg.qos        = MQTT_QOS;
    msg.retained   = 0;

    int rc = MQTTClient_publishMessage(ctx->handle, ctx->topic, &msg, &token);
    if (rc != MQTTCLIENT_SUCCESS) {
        ctx->connected = false;
        fprintf(stderr, "mqtt: publish failed: %d\n", rc);
        return -1;
    }
    return MQTTClient_waitForCompletion(ctx->handle, token, 5000);
}

void mqtt_disconnect(mqtt_ctx_t *ctx) {
    if (ctx->connected) {
        MQTTClient_disconnect(ctx->handle, 1000);
        ctx->connected = false;
    }
    MQTTClient_destroy(&ctx->handle);
}

bool mqtt_is_connected(mqtt_ctx_t *ctx) { return ctx->connected; }

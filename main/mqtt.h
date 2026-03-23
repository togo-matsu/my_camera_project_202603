#ifndef MQTT_H
#define MQTT_H

// MQTT broker / client configuration (defaults can be overridden at build)
#ifndef MQTT_BROKER_URI
#define MQTT_BROKER_URI ""
#endif

// Subscribe topic for LED control
#ifndef MQTT_SUBSCRIBE_TOPIC
#define MQTT_SUBSCRIBE_TOPIC "/test/topic/led"
#endif

// GPIO used for LED control (can be overridden at build)
#ifndef MQTT_LED_GPIO
#define MQTT_LED_GPIO GPIO_NUM_21
#endif

// AWSで登録した「モノ」の名前
#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID "mydevice" 
#endif

// Default publish parameters
#ifndef MQTT_PUBLISH_QOS
#define MQTT_PUBLISH_QOS 1
#endif

#ifndef MQTT_PUBLISH_RETAIN
#define MQTT_PUBLISH_RETAIN 0
#endif

// Default topic used by mqtt_task when NULL is passed
#ifndef MQTT_DEFAULT_TOPIC
#define MQTT_DEFAULT_TOPIC "/test/topic/"
#endif

int mqtt_publish(const char *topic, const char *data);
void mqtt_init(void);
void mqtt_task_start(void);

#endif // MQTT_H

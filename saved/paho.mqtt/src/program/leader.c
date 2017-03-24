#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include "MQTTClient.h"

#define ADDRESS     "tcp://192.168.1.200:1883"
#define CLIENTID    "robot_1"
#define TOPIC       "INFO"
#define QOS         1
#define TIMEOUT     10000L

struct Info //This structure should be in a header file
{
   int velocity;
   int direction;
   int location_x;
   int location_y;
   int acceleration;
};

struct Info publish_info = { 1, 2, 3, 4, 5 }; //Struct initialize with default info
char buffer[128];

void print_info(struct Info s) {
   snprintf(buffer, sizeof(buffer), "{\"velocity\":%d,\"direction\":%d,\"location_x\":%d,\"location_y\":%d,\"acceleration\":%d}", s.velocity,s.direction,s.location_x,s.location_y,s.acceleration);
}

int main(int argc, char* argv[])
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message msg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    print_info(publish_info);
    msg.payload = buffer;
    msg.payloadlen = sizeof(buffer);
    msg.qos = QOS;
    msg.retained = 0;
    MQTTClient_publishMessage(client, TOPIC, &msg, &token); //Publish message
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);

    MQTTClient_disconnect(client, 10000); //Disconnects the client in a timeout of 10 seconds
    MQTTClient_destroy(&client);
    return rc;
}

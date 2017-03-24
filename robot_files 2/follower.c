#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"

#include "pthread.h" //To can use pthread_mutex_t
#include "unistd.h"

#include <json-c/json.h>

#define ADDRESS     "tcp://192.168.1.249:1883"
#define CLIENTID    "robot_2"
#define TOPIC       "INFO"
#define QOS         1
#define TIMEOUT     10000L

int velocity = 0, new_velocity = 0;
int direction = 0, new_direction = 0;
int location_x = 0, new_location_x = 0;
int location_y = 0, new_location_y = 0;
int acceleration = 0, new_acceleration = 0;

volatile MQTTClient_deliveryToken deliveredtoken;
pthread_mutex_t lock;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    pthread_mutex_lock(&lock);
    char* payloadptr;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    printf("%s\n", message->payload);  //Show message --> Not need it at the end just to test it
    //Get values from message and save them into the variables (new_velocity, new_direction, ...)
    json_object * jobj = json_tokener_parse(message->payload);     
    new_velocity = json_object_get_int(json_object_object_get(jobj, "velocity"));
    new_direction = json_object_get_int(json_object_object_get(jobj, "direction"));
    new_location_x = json_object_get_int(json_object_object_get(jobj, "location_x"));
    new_location_y = json_object_get_int(json_object_object_get(jobj, "location_y"));
    new_acceleration = json_object_get_int(json_object_object_get(jobj, "acceleration"));
    printf("\nnew_velocity=%d\nnew_direction=%d\nnew_location_x=%d\nnew_location_y=%d\nnew_acceleration=%d\n\n", new_velocity, new_direction,new_location_x,new_location_y,new_acceleration);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    pthread_mutex_unlock(&lock); 
    return 1;
}

int update_info(){ //Updates info variables to control the robots with the new information
   velocity = new_velocity;
   direction = new_direction;
   location_x = new_location_x;
   location_y = new_location_y;
   acceleration = new_acceleration;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[])
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;

    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);
    
    do 
    {
        ch = getchar();
    } while(ch!='Q' && ch != 'q');

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}

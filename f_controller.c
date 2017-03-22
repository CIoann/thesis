/*
     ____ __     ____   ___    ____ __         (((((()
    | |_  \ \  /   ) ) | |  ) | |_  \ \  /  \(@)- /
    |_|__  \_\/  __)_) |_|_/  |_|__  \_\/   /(@)- \
                                               ((())))
 *//**
 *  \file  test.c
 *  \brief  ev3dev-c drive around
 *  \author Constantina Ioannou (ioannou.connie@gmail.com)
 *  \copyright  See the LICENSE file.
 */

#include <stdio.h>
#include "coroutine.h"
#include "ev3.h"
#include "ev3_port.h"
#include "ev3_sensor.h"
#include "ev3_tacho.h"

#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"

#include "pthread.h" //To can use pthread_mutex_t
#include "unistd.h"

#include <json-c/json.h>

#define ADDRESS     "tcp://192.168.1.250:1883"
#define CLIENTID    "robot_2"
#define TOPIC       "INFO"
#define QOS         1
#define TIMEOUT     10000L

int velocity = 0, new_velocity = 0;
int direction = 0, new_direction = 0;
int location_x = 0, new_location_x = 0;
int location_y = 0, new_location_y = 0;
int acceleration = 0, new_acceleration = 0;

// WIN32 /////////////////////////////////////////
#ifdef __WIN32__

#include <windows.h>

// UNIX //////////////////////////////////////////
#else

#include <unistd.h>
#define Sleep( msec ) usleep(( msec ) * 1000 )

//////////////////////////////////////////////////
#endif

#define L_MOTOR_PORT      OUTPUT_A
#define L_MOTOR_EXT_PORT  EXT_PORT__NONE_
#define R_MOTOR_PORT      OUTPUT_B
#define R_MOTOR_EXT_PORT  EXT_PORT__NONE_
#define IR_CHANNEL        0

#define SPEED_LINEAR      5  /* Motor speed for linear motion, in percents */
#define SPEED_CIRCULAR    20  /* ... for circular motion */
#define _MAX_SPEED_ 100
int max_speed;  /* Motor maximal speed */

#define DEGREE_TO_COUNT( d )  (( d ) * 260 / 90 )

int app_alive;


typedef struct{
	int velocity;
	float locX;
	float locY;
	float orientation;
	int acceleration;
	int id;
	int mode;
} VEHICLE_PROFILE;

VEHICLE_PROFILE vp;

enum {
	MODE_LEADER,  /* Supervisory - follow the line */
	MODE_FOLLOWER, /* Follower - Follow the front vehicle */
	MODE_AUTO,    /* Self-driving */
};


enum {
	MOVE_NONE,
	MOVE_FORWARD,
	MOVE_BACKWARD,
	TURN_LEFT,
	TURN_RIGHT,
	TURN_ANGLE,
	STEP_BACKWARD,
	STEP_FORWARD,
};

const char *color[] = { "?", "BLACK", "BLUE", "GREEN", "YELLOW", "RED", "WHITE", "BROWN" };
#define COLOR_COUNT  (( int )( sizeof( color ) / sizeof( color[ 0 ])))
uint8_t sn_colour;

int moving;   /* Current moving */
int command;  /* Command for the 'drive' coroutine */
int angle;    /* Angle of rotation */

uint8_t  ir,touch;  /* Sequence numbers of sensors */
enum { L, R };
uint8_t motor[ 3 ] = { DESC_LIMIT, DESC_LIMIT, DESC_LIMIT };  /* Sequence numbers of motors */


static void _set_mode( int value )
{
	//choose mode! wifi
		vp.mode = MODE_LEADER;

}

static void _run_forever( int l_speed, int r_speed )
{
	set_tacho_speed_sp( motor[ L ], l_speed );
	set_tacho_speed_sp( motor[ R ], r_speed );
	multi_set_tacho_command_inx( motor, TACHO_RUN_FOREVER );
}

static void _run_to_rel_pos( int l_speed, int l_pos, int r_speed, int r_pos )
{
	set_tacho_speed_sp( motor[ L ], l_speed );
	set_tacho_speed_sp( motor[ R ], r_speed );
	set_tacho_position_sp( motor[ L ], l_pos );
	set_tacho_position_sp( motor[ R ], r_pos );
	multi_set_tacho_command_inx( motor, TACHO_RUN_TO_REL_POS );
}

static void _run_timed( int l_speed, int r_speed, int ms )
{
	set_tacho_speed_sp( motor[ L ], l_speed );
	set_tacho_speed_sp( motor[ R ], r_speed );
	multi_set_tacho_time_sp( motor, ms );
	multi_set_tacho_command_inx( motor, TACHO_RUN_TIMED );
}

static int _is_running( void )
{
	FLAGS_T state = TACHO_STATE__NONE_;

	get_tacho_state_flags( motor[ L ], &state );
	if ( state != TACHO_STATE__NONE_ ) return ( 1 );
	get_tacho_state_flags( motor[ R ], &state );
	if ( state != TACHO_STATE__NONE_ ) return ( 1 );

	return ( 0 );
}

static void _stop( void )
{
	multi_set_tacho_command_inx( motor, TACHO_STOP );
}



// ===========================================
/* sensor connection established functions */
// ===========================================

int app_init( void )
{
	char s[ 16 ];
	/*check left motor*/
	if ( ev3_search_tacho_plugged_in( L_MOTOR_PORT, L_MOTOR_EXT_PORT, motor + L, 0 )) {
		get_tacho_max_speed( motor[ L ], &max_speed );
		/* Reset the motor */
		set_tacho_command_inx( motor[ L ], TACHO_RESET );
	} else {
		printf( "LEFT motor (%s) is NOT found.\n", ev3_port_name( L_MOTOR_PORT, L_MOTOR_EXT_PORT, 0, s ));
		/* Inoperative without left motor */
		return ( 0 );
	}

	//check right motor

	if ( ev3_search_tacho_plugged_in( R_MOTOR_PORT, R_MOTOR_EXT_PORT, motor + R, 0 )) {
		/* Reset the motor */
		set_tacho_command_inx( motor[ R ], TACHO_RESET );
	} else {
		printf( "RIGHT motor (%s) is NOT found.\n", ev3_port_name( R_MOTOR_PORT, R_MOTOR_EXT_PORT, 0, s ));
		/* Inoperative without right motor */
		return ( 0 );
	}
	// check color sensor
	if (ev3_search_sensor ( LEGO_EV3_COLOR, &sn_colour, 0 )){
		
		set_sensor_mode(sn_colour, "COL-COLOR");

	} else {
		printf( "Color Sensor () is NOT found.\n" );
		/* Inoperative without color sensor */
		return ( 0 );
	}
	command	= moving = MOVE_NONE;

	return ( 1 );
}




// ===========================================
/* velocity functions */
// ===========================================
/* MAX SPEED?*/
bool _is_speed_max(int cspeed){
	if (cspeed<_MAX_SPEED_){
		return true;
	}
	return false;
}
/* decelerate */ 
void decelerate(){
vp.velocity = 10;

}
/* Accelerate */
void accelerate(int current_speed, int* nspeed){
	if (_is_speed_max(current_speed)){
			
			vp.velocity = vp.velocity + 10;
			*nspeed = max_speed*(current_speed+SPEED_LINEAR)/100;
			_run_forever(*nspeed,*nspeed);
			//command = MOVE_BACKWARD;
			}
}
/*set desired speed */
void  set_speed(int dspeed){
	vp.velocity = dspeed;
	_run_forever(dspeed,dspeed);
}

// ====================
// FOLLOWER FUNCTIONS
// ====================
/* receive location from ultra sonic */
float FusDistance(){
	return 0.1;
	//return -1 if no one in range
}

void FFollow(){
	// if f_detectLeader 

		// while ! (v1 <f_usDistance < v2) 
		// then accelerate
		// otherwise continue with the same speed.
	//else 
		// start select algorithm
}
bool FdetectLeader(){
	// if (f_usDistance < 0)
	return true;
}
void FbecomeLeader(){
	//receive info of how many cars should pass
	//move to the front of the platoon
}

// ====================
// LEADER FUNCTIONS
// ====================
void Lmovement(){
	//detect obstacles
	//detect turns
	//follow black line
	// accelerate/decelerate
}

void LsearchObstacles(){
	//ir sensor
}
void Lsearchline(){
	//find the black line 
}

void LchangeLeader(){
	// send a message to the follower behind to take leadership
	// exit platoon
}



CORO_CONTEXT( handle_color );

CORO_DEFINE ( handle_color )
{
	CORO_LOCAL int val;
	CORO_LOCAL int nspeed=10;
	CORO_BEGIN();
	for( ; ; ){
		CORO_WAIT(get_sensor_value(0, sn_colour, &val ) || ( val > 0 ) || ( val <= COLOR_COUNT ));
		printf( "\r(%s)", color[ val ]);
		fflush( stdout );
		if (val == 1) {
			accelerate(vp.velocity,&nspeed);
		}else{
			vp.velocity =5;
			nspeed = max_speed*(5+SPEED_LINEAR)/100;
			_run_forever(nspeed,nspeed);
		}
         	CORO_YIELD();
                update_info();
	}
	CORO_END();
}


void init_vp(){
	vp.id =5;
	vp.orientation = 0.0;
	vp.mode = MODE_LEADER;
	vp.locX = 0.0;
	vp.locY = 0.0;
	vp.velocity = 10;
}

//////////////////////////////////////////////
//         COMMUNICATION FUNCTIONS          //
//////////////////////////////////////////////

int create_mqtt_communication(){
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
        return -1;
    }
    return 1;
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

//VEHICLE_PROFILE vp;
int main( void )
{
init_vp();
/*	printf( "Waiting the EV3 brick online...\n" );
	if ( ev3_init() < 1 ) return ( 1 );

	printf( "*** ( EV3 ) Hello! ***\n" );
	printf( "*** I am: %d and I am in mode:", vp.id);
*/	ev3_sensor_init();
	ev3_tacho_init();
	app_alive = app_init();

        if (create_mqtt_communication == -1)
        {
            printf("Failed to connect, return code %d\n", rc);
            exit(EXIT_FAILURE);
        }
        printf("Subscribing to topic %s for client %s using QoS%d\n\n", TOPIC, CLIENTID, QOS);
        MQTTClient_subscribe(client, TOPIC, QOS);

	while ( app_alive ) {
//		printf("hello %d",i);
		CORO_CALL( handle_color );
	}

	ev3_uninit();
        MQTTClient_disconnect(client, 10000);
        MQTTClient_destroy(&client);
	printf( "*** ( EV3 ) Bye! ***\n" );

	return ( 0 );
}

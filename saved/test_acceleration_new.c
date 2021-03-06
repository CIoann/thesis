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
#include "ev3_transfer.h"

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

int max_speed;  /* Motor maximal speed */

#define DEGREE_TO_COUNT( d )  (( d ) * 260 / 90 )

int app_alive;

int mode;  /* Driving mode */
int speed=10; /* Driving speed*/



const char *color[] = { "?", "BLACK", "BLUE", "GREEN", "YELLOW", "RED", "WHITE", "BROWN" };
#define COLOR_COUNT  (( int )( sizeof( color ) / sizeof( color[ 0 ])))
uint8_t sn_colour;

int moving;   /* Current moving */
int command;  /* Command for the 'drive' coroutine */
int angle;    /* Angle of rotation */

uint8_t  ir,touch;  /* Sequence numbers of sensors */

uint8_t motor[ 3 ] = { DESC_LIMIT, DESC_LIMIT, DESC_LIMIT };  /* Sequence numbers of motors */




// ===========================================
/* sensor connection established functions */
// ===========================================

/* Check if infrared is connected */
int _is_IR_connected( void ){
	if ( ev3_search_sensor( LEGO_EV3_IR, &ir, 0 )) {
		_set_mode( MODE_REMOTE );
	} else {
		printf( "IR sensor is NOT found.\n" );
		/* Inoperative without IR sensor */
		return ( 0 );
	}
	return ( 1 );
}
/* Check if ultrasonic is connected */
int _is_ultrasonic_connected(void){
	return 1;
}
/* Check if sensor color is connected */
int _is_sn_color_connected(void){
	if (ev3_search_sensor ( LEGO_EV3_COLOR, &sn_colour, 0 )){
		
		set_sensor_mode(sn_colour, "COL-COLOR");

	} else {
		printf( "Color Sensor () is NOT found.\n" );
		/* Inoperative without color sensor */
		return ( 0 );
	}

	return 1;
}
/* Check if motors are connected */
int _is_Tacho_connected(void){
	
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

	return 1;
}

// ===========================================
/* velocity functions */
// ===========================================
/* MAX SPEED?*/
int is_speed_max(){
	/*if (speed>_MAX_SPEED_){
		return 0;
	}*/
	return 1;
}
/* decelerate */ 
void decelerate(){

}
/* Accelerate */
void accelerate(){
	// if (_is_speed_max()){
	// 	speed = speed + 10;
	// 	nspeed = max_speed*(speed+SPEED_LINEAR)/100;
	// 		_run_forever(nspeed,nspeed);
	// }
	// _run_forever (speed, speed);
}
/* Set Movement speed */
void _run_forever(int l_speed, int r_speed){
	set_tacho_speed_sp( motor[ L ], l_speed );
	set_tacho_speed_sp( motor[ R ], r_speed );
	multi_set_tacho_command_inx( motor, TACHO_RUN_FOREVER );
}
/* Turn Right */ 
void _run_turnRight(int l_speed, int r_speed){
	_run_forever( -speed_circular, speed_circular );
}
/* Turn Left */
void _run_turnLeft(int l_speed, int r_speed){
	_run_forever( speed_circular, -speed_circular );
}
/* Movement specific mili seconds */
void _run_timed( int l_speed, int r_speed, int ms ){
	set_tacho_speed_sp( motor[ L ], l_speed );
	set_tacho_speed_sp( motor[ R ], r_speed );
	multi_set_tacho_time_sp( motor, ms );
	multi_set_tacho_command_inx( motor, TACHO_RUN_TIMED );
}
/* is it running? */
int _is_running( void ){
	FLAGS_T state = TACHO_STATE__NONE_;

	get_tacho_state_flags( motor[ L ], &state );
	if ( state != TACHO_STATE__NONE_ ) return ( 1 );
	get_tacho_state_flags( motor[ R ], &state );
	if ( state != TACHO_STATE__NONE_ ) return ( 1 );

	return ( 0 );
}
/* Stop tachos */
void _stop( void ){
	multi_set_tacho_command_inx( motor, TACHO_STOP );
}


/* Choose leader or follower*/
void _set_mode (int value){
	mode = value;
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

/* Check i APP INIT is established */ 
int app_init( void ){
	if (_is_Tacho_connected() && _is_sn_color_connected() && _is_IR_connected() && _is_ultrasonic_connected()){
		//command	= moving = MOVE_NONE;
		return ( 1 );
	}else{
		return ( 0 );
	}
}








CORO_CONTEXT( handle_color );


CORO_DEFINE ( handle_color )
{
	CORO_LOCAL int val;
	CORO_LOCAL int nspeed=10;
	CORO_BEGIN();
//for ( ; ; ){
//	if (sn_colour == DESC_LIMIT ) CORO_QUIT();
for( ; ; ){
		CORO_WAIT(get_sensor_value(0, sn_colour, &val ) || ( val > 0 ) || ( val <= COLOR_COUNT ));
		printf( "\r(%s)", color[ val ]);
		fflush( stdout );
		if (val == 1) {
			if (speed<100){
			speed  = speed +10;
			nspeed = max_speed*(speed+SPEED_LINEAR)/100;
			_run_forever(nspeed,nspeed);
			//command = MOVE_BACKWARD;
			}
//			break;
		}else{
			speed =0;

			nspeed = max_speed*(5+SPEED_LINEAR)/100;
			_run_forever(nspeed,nspeed);
//			command = MOVE_BACKWARD;
}
	CORO_YIELD();
	}
CORO_YIELD();
	CORO_END();
}





int main( void )
{
	printf( "Waiting the EV3 brick online...\n" );
	if ( ev3_init() < 1 ) return ( 1 );

	printf( "*** ( EV3 ) Hello! ***\n" );
	ev3_sensor_init();
	ev3_tacho_init();

	app_alive = app_init();
	while ( app_alive ) {
		CORO_CALL( handle_color );

	}

	ev3_uninit();
	printf( "*** ( EV3 ) Bye! ***\n" );

	return ( 0 );
}

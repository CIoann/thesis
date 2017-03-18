/*
     ____ __     ____   ___    ____ __         (((((()
    | |_  \ \  /   ) ) | |  ) | |_  \ \  /  \(@)- /
    |_|__  \_\/  __)_) |_|_/  |_|__  \_\/   /(@)- \
                                               ((())))
 *//**
 /*  ev3_transfer.h 
 *//**
 *  \file  ev3_transfer.h
 *  \brief  EV3 creating transfer wireless info
 *  \author  Constantina Ioannou (ioannou.connie@gmail.com)
 */

#include "ev3_transfer.h"  
#define COLOR_COUNT  (( int )( sizeof( color ) / sizeof( color[ 0 ])))
#define L_MOTOR_PORT      OUTPUT_A
#define L_MOTOR_EXT_PORT  EXT_PORT__NONE_
#define R_MOTOR_PORT      OUTPUT_B
#define R_MOTOR_EXT_PORT  EXT_PORT__NONE_
#define IR_CHANNEL        0

#define SPEED_LINEAR      5  /* Motor speed for linear motion, in percents */
#define SPEED_CIRCULAR    20  /* ... for circular motion */

uint8_t motor[ 3 ] = { DESC_LIMIT, DESC_LIMIT, DESC_LIMIT };  /* Sequence numbers of motors */
const char *color[] = { "?", "BLACK", "BLUE", "GREEN", "YELLOW", "RED", "WHITE", "BROWN" };



// ===========================================
/* sensor connection established functions */
// ===========================================

/* Check if infrared is connected */
int _is_IR_connected(uint8_t ir ){
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
	if (_is_Tacho_connected && _is_sn_color_connected && _is_IR_connected && _is_ultrasonic_connected){
		command	= moving = MOVE_NONE;
		return ( 1 );
	}else{
		return ( 0 );
	}
}
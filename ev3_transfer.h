
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

#ifndef EV3_TRANSFER_H
#define EV3_TRANSFER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


/** 
* \brief Structure of the Vehicle Profile 
*/

typedef struct {
	float x;
	float y;

}Coords;

typedef struct{
	int velocity;
	Coords location;
	float orientation;
	int acceleration;
	int role;
	int id;
	int mode;
} VEHICLE_PROFILE;


enum {
	MODE_LEADER,  /* Supervisory  */
	MODE_FOLLOWER, /* Follower - Follow the front vehicle */
	MODE_AUTO,    /* Self-driving */
};

enum { L, R };

enum {
	MOVE_NONE,
	MOVE_FORWARD,
	MOVE_BACKWARD,
	TURN_LEFT,
	TURN_RIGHT,
	TURN_ANGLE,
	STEP_BACKWARD,
};



/* Check i APP INIT is established */ 
int app_init( void );

// ===========================================
/* sensor connection established functions */
// ===========================================

int _is_IR_connected( void );
/* Check if ultrasonic is connected */
int _is_ultrasonic_connected(void);
/* Check if sensor color is connected */
int _is_sn_color_connected(void);	
/* Check if motors are connected */
int _is_Tacho_connected(void);



// ===========================================
/* velocity functions */
// ===========================================
/* MAX SPEED?*/
int is_speed_max();
/* decelerate */ 
void decelerate();
/* Accelerate */
void accelerate();
/* Set Movement speed */
void _run_forever(int l_speed, int r_speed);
/* Turn Right */ 
void _run_turnRight(int l_speed, int r_speed);
/* Turn Left */
void _run_turnLeft(int l_speed, int r_speed);
/* Movement specific mili seconds */
void _run_timed( int l_speed, int r_speed, int ms );
/* is it running? */
int _is_running( void );
/* Stop tachos */
void _stop( void );


// ===========================
/* Choose leader or follower*/
// ===========================
void _set_mode (int value){
	mode = value;
}

// ====================
// FOLLOWER FUNCTIONS
// ====================
/* receive location from ultra sonic */
float FusDistance();
void FFollow();
bool FdetectLeader();
void FbecomeLeader();

// ====================
// LEADER FUNCTIONS
// ====================
void Lmovement();
void LsearchObstacles();
void Lsearchline();
void LchangeLeader();


#endif
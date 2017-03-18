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

enum {
	MODE_LEADER,  /* Supervisory - follow the line */
	MODE_FOLLOWER, /* Follower - Follow the front vehicle */
	MODE_AUTO,    /* Self-driving */
};

int mode;  /* Driving mode */
int speed=10; /* Driving speed*/

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
		mode = MODE_LEADER;

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

CORO_CONTEXT( handle_touch );
CORO_CONTEXT( handle_color );
CORO_CONTEXT( handle_brick_control );
//CORO_CONTEXT( handle_supervisory );
CORO_CONTEXT( drive );
/* Coroutine of the TOUCH sensor handling */
CORO_DEFINE( handle_touch )
{
	CORO_LOCAL int val;

	CORO_BEGIN();
	if ( touch == DESC_LIMIT ) CORO_QUIT();

	for ( ; ; ) {
		/* Waiting the button is pressed */
		CORO_WAIT( get_sensor_value( 0, touch, &val ) && ( val ));
		/* Stop the vehicle */
		command = MOVE_BACKWARD;
		/* Switch mode */
		_set_mode(( mode == MODE_LEADER ) ? MODE_AUTO : MODE_LEADER );
		/* Waiting the button is released */
		CORO_WAIT( get_sensor_value( 0, touch, &val ) && ( !val ));
	}
	CORO_END();
}

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



/* Coroutine of the EV3 brick keys handling 
	define leader and follower manually*/
CORO_DEFINE( handle_brick_control )
{
	CORO_LOCAL uint8_t keys, pressed = EV3_KEY__NONE_;

	CORO_BEGIN();
	for ( ; ; ) {
		/* Waiting any key is pressed or released */
		CORO_WAIT( ev3_read_keys( &keys ) && ( keys != pressed ));
		pressed = keys;

		if ( pressed & EV3_KEY_BACK ) {
			command = MOVE_NONE;
			_set_mode(( mode == MODE_FOLLOWER ) ? MODE_AUTO : MODE_FOLLOWER );


		} else if ( pressed & EV3_KEY_UP ) {
			/* Stop the vehicle */
			command = MOVE_NONE;
			/* Switch mode */
			_set_mode(( mode == MODE_LEADER ) ? MODE_AUTO : MODE_LEADER );
		}
		CORO_YIELD();
	}
	CORO_END();
}

/* Coroutine of control the motors */
CORO_DEFINE( drive )
{
	CORO_LOCAL int speed_linear, speed_circular;
	CORO_LOCAL int _wait_stopped;

	CORO_BEGIN();
	speed_linear = (max_speed *(speed+ SPEED_LINEAR)) / 100;
	speed_circular = max_speed * SPEED_CIRCULAR / 100;

	for ( ; ; ) {
		/* Waiting new command */
		CORO_WAIT( moving != command );
//§§§§§§§§speed_linear = (max_speed*(speed+SPEED_LINEAR))/100;

		_wait_stopped = 0;
		switch ( command ) {

		case MOVE_NONE:
			_stop();
			_wait_stopped = 1;
			break;

		case MOVE_FORWARD:
			_run_forever( -speed_linear, -speed_linear );
			break;

		case MOVE_BACKWARD:
			_run_forever( speed_linear, speed_linear );
			break;

		case TURN_LEFT:
			_run_forever( speed_circular, -speed_circular );
			break;

		case TURN_RIGHT:
			_run_forever( -speed_circular, speed_circular );
			break;

		case TURN_ANGLE:
			_run_to_rel_pos( speed_circular, DEGREE_TO_COUNT( -angle )
			, speed_circular, DEGREE_TO_COUNT( angle ));
			_wait_stopped = 1;
			break;

		case STEP_BACKWARD:
			_run_timed( speed_linear, speed_linear, 1000 );
			_wait_stopped = 1;
			break;
		case STEP_FORWARD:
			_run_timed(-speed_linear,-speed_linear,1000);
			_wait_stopped = 1;
			break;
		}

		moving = command;

		if ( _wait_stopped ) {
			/* Waiting the command is completed */
			CORO_WAIT( !_is_running());

			command = moving = MOVE_NONE;
			speed_linear = max_speed*(speed+SPEED_LINEAR)/100;
		}
	}
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

		CORO_CALL( handle_touch );
		CORO_CALL( handle_color );
		CORO_CALL( handle_brick_control );
		/*if (mode == MODE_LEADER){
			CORO_CALL( supervisory_drive );

			//FOLLOW THE LINE
		}else{
			//FOLLOW THE VEHICLE IN FRONT
		}*/

	}

	ev3_uninit();
	printf( "*** ( EV3 ) Bye! ***\n" );

	return ( 0 );
}

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

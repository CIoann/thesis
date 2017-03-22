# ev3dev_platoon

1. In order to get access in the robots sensors and motors you need to download this repository on the robots "https://github.com/in4lio/ev3dev-c" 
2. Create a folder in directory eg/
3. Place the c file I included in the new directory. In the new directory along with the file you need to copy paste a makefile. This makefile you can find in the other folders for exapmle eg/tacho. In the new directory you created you should also create two folders named as "Debug" and "Release".
4. By running make you should be able to run the program.


Now to combine you only need to take care that you must have only one file which has the main function of the robot.

Communication part code is in paho.mqtt.c/src/program. There are two programs leader.c and follower.c

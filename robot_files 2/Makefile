JSON_C_DIR=/usr/local
CFLAGS += -I$(JSON_C_DIR)/include/* -I/paho.mqtt.c/src/*.h
LDFLAGS+= -L$(JSON_C_DIR)/lib -ljson-c -lpthread -ldl -lssl -lcrypto

MQTT= paho.mqtt.c/src/*.c
EV3DEV = ev3dev-c/src/*.c

all: f_controller l_controller leader follower test_acceleration

f_controller: f_controller.c
	gcc f_controller.c $(MQTT) $(EV3DEV) $(CFLAGS) $(LDFLAGS) -o f_controller

l_controller: l_controller.c
	gcc l_controller.c $(MQTT) $(EV3DEV) $(CFLAGS) $(LDFLAGS) -o l_controller

leader: leader.c
	gcc leader.c $(MQTT) $(CFLAGS) $(LDFLAGS) -o leader

follower: follower.c
	gcc follower.c $(MQTT) $(CFLAGS) $(LDFLAGS) -o follower

test_acceleration: test_acceleration.c
	gcc test_acceleration.c $(EV3DEV) $(CFLAGS) $(LDFLAGS) -o test_acceleration




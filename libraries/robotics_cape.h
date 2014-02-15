#ifndef ROBOTICS_CAPE
#define ROBOTICS_CAPE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>	//usleep, nanosleep
#include <math.h>	//atan2 and fabs
#include <signal.h>	//capture ctrl-c
#include <pthread.h>   // multi-threading
#include <linux/input.h> //button and interrupt events


#include "SimpleGPIO.h"
#include "mpu9150.h"



//User available functions
int initialize_cape();
int set_motor(int motor, float duty);
int set_pwm_period_ns(int period);
long int get_encoder(int encoder);
int getStartBTN();
int getSelectBTN();
int setGRN(PIN_VALUE i);
int setRED(PIN_VALUE i);
float getBattVoltage();
void* spektrum_read(void* ptr);
float spektrum_input_scaled(int channel);
int spektrum_input_raw(int channel);
int cleanup_cape();

int set_imu_interrupt_func(int (*func)(void));
int set_start_pressed_func(int (*func)(void));
int set_start_unpressed_func(int (*func)(void));
int set_select_pressed_func(int (*func)(void));
int set_select_unpressed_func(int (*func)(void));
int get_start_button();
int get_select_button();
void* read_events(void* ptr);
int null_func();

enum state_t {UNINITIALIZED,RUNNING,PAUSED,EXITING};
enum state_t state = UNINITIALIZED;
enum state_t get_state();
int set_state(enum state_t);

void ctrl_c(int signo);

#define RC_CHANNELS 9
#define SPEKTRUM_CAL_FILE "/home/root/cape_calibration/spektrum.cal"
int calibrate_spektrum();
int initialize_spektrum();
float get_rc_channel(int ch);

int get_rc_new_flag();
const char *byte_to_binary(int x);
void cleanup(int signo);
void* uart4_checker(void *ptr);

int initialize_imu(int sample_rate);
#endif


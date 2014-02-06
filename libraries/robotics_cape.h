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

/* #define SYSFS_GPIO_DIR "/sys/class/gpio"
#define POLL_TIMEOUT (3 * 1000) // 3 seconds
#define MAX_BUF 64
#define SYSFS_OMAP_MUX_DIR "/sys/kernel/debug/omap_mux/" */

/* typedef enum {
	INPUT_PIN,
	OUTPUT_PIN
}PIN_DIRECTION;

typedef enum {
	LOW,
	HIGH
} PIN_VALUE;

 */

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
#endif


/* 
//supporting functions
int gpio_fd_open(unsigned int gpio);
int gpio_set_value;
int gpio_set_value(unsigned int gpio, PIN_VALUE value);
int gpio_fd_close(int fd);
int gpio_omap_mux_setup(const char *omap_pin0_name, const char *mode); */
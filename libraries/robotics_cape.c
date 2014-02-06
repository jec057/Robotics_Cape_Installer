/*
Supporting library for Robotics Cape Features
Strawson Design - 2013
*/

#include "robotics_cape.h"

///////////////////////////////////////////////////
//////////////////  STATE VARIABLE  ///////////////
///////////////////////////////////////////////////
//control the flow of your loops and threads with this

enum state_t get_state(){
	return state;
}

int set_state(enum state_t new_state){
	state = new_state;
}


///////////////////////////////////////////////////
////////////////////  OUTPUT PINS   ///////////////
///////////////////////////////////////////////////
// gpio number is first digit *32 plus second digit
#define MDIR1A    20	//gpio0.20
#define MDIR1B    112	//gpio3.16
#define MDIR2A    113	//gpio3.17
#define MDIR2B    61	//gpio1.29
#define MDIR3A    49	//gpio1.17
#define MDIR3B    48	//gpio1.16
#define MDIR4A    65	//gpio2.1 
#define MDIR4B    27	//gpio0.27 
#define MDIR5A    26	//gpio0.26
#define MDIR5B    68	//gpio1.28
#define MDIR6A    68	//gpio2.4
#define MDIR6B    66	//gpio2.2
#define GRN_LED 47	// gpio1.15 "P8_15"
#define RED_LED 46	// gpio1.14 "P8_16"
//Spektrum UART4 RX must be remuxed to gpio output temporarily for pairing
#define PAIRING_PIN 30 //P9.11 gpio0.30
#define NUM_OUT_PINS 15
unsigned int out_gpio_pins[] = {MDIR1A, MDIR1B, MDIR2A, MDIR2B, 
								 MDIR3A, MDIR3B, MDIR4A, MDIR4B, 
								 MDIR5A, MDIR5B, MDIR5A, MDIR5B,
								 GRN_LED, RED_LED, PAIRING_PIN};

								 
	

	
///////////////////////////////////////////////////
////////////////////  INPUT PINS   ////////////////
///////////////////////////////////////////////////
#define START_BTN 67	//gpio2.3 P8_8
#define SELECT_BTN 69	//gpio2.6 P8_9
#define INTERRUPT_PIN 121  //gpio3.21 P9.25

int start_btn_state, select_btn_state;
int (*imu_interrupt_func)();
int (*start_unpressed_func)();
int (*start_pressed_func)();
int (*select_unpressed_func)();
int (*select_pressed_func)();

//function pointers for events initialized to null_fun
//instead of containing a null pointer
int null_func(){
	return 0;
}

int set_imu_interrupt_func(int (*func)(void)){
	imu_interrupt_func = func;
	return 0;
}
int set_start_pressed_func(int (*func)(void)){
	start_pressed_func = func;
	return 0;
}
int set_start_unpressed_func(int (*func)(void)){
	start_unpressed_func = func;
	return 0;
}
int set_select_pressed_func(int (*func)(void)){
	select_pressed_func = func;
	return 0;
}
int set_select_unpressed_func(int (*func)(void)){
	select_unpressed_func = func;
	return 0;
}

int get_start_button(){
	return start_btn_state;
}
int get_select_button(){
	return select_btn_state;
}



///////////////////////////////////////////////////
////////////////////  PWM PINS   //////////////////
///////////////////////////////////////////////////
#define DEFAULT_PERIOD_NS    500000  //pwm period nanoseconds: 20khz
char pwm_files[][MAX_BUF] = {"/sys/devices/ocp.3/pwm_test_P9_31.12/",
							 "/sys/devices/ocp.3/pwm_test_P9_29.13/",
							 "/sys/devices/ocp.3/pwm_test_P9_14.15/",
							 "/sys/devices/ocp.3/pwm_test_P9_16.14/",
							 "/sys/devices/ocp.3/pwm_test_P8_19.16/",
							 "/sys/devices/ocp.3/pwm_test_P8_13.17/"
};
FILE *pwm_duty_pointers[6]; //store pointers to 6 pwm channels for frequent writes
int pwm_period_ns; //stores current pwm period in nanoseconds

///////////////////////////////////////////////////
/////////////   eQEP Encoder Stuff   //////////////
///////////////////////////////////////////////////
char encoder_files[][MAX_BUF] = {"/sys/devices/ocp.3/48300000.epwmss/48300180.eqep/position",
								 "/sys/devices/ocp.3/48302000.epwmss/48302180.eqep/position",
								 "/sys/devices/ocp.3/48304000.epwmss/48304180.eqep/position"					
};
FILE *encoder_pointers[3]; //store pointers to 3 encoder channels for frequency reads
/*	
FILE *eqep0;
FILE *eqep1;
FILE *eqep2;
*/



///////////////////////////////////////////////////
//////  ADC AIN6 for Battery Monitoring    ////////
///////////////////////////////////////////////////
FILE *AIN6_file;
int millivolts;	


///////////////////////////////////////////////////
//////////   UART4 Spektrum Stuff   ///////////////
///////////////////////////////////////////////////
#define RC_CHANNELS 9
int rc_channels[RC_CHANNELS];
char j[RC_CHANNELS*2+2]; // possible 9 channels, 2 bytes each. plus 2 preamble bytes
int uart4_port,ret;
int spektrum_means[] = {3070, 11262, 5118, 8498, 7166, 12288, 306, 38023,0};
int spektrum_mins[] = {2352, 11262, 5836, 8498, 6448, 12288, 306, 38023,0};
int spektrum_maxs[] = {3786, 11262, 4402, 8498, 7882, 12288, 306, 38023,0};




int initialize_cape(){
	set_state(RUNNING);
	
	printf("\n\nEnabling exit signal handler\n");
	signal(SIGINT, ctrl_c);	
	
	printf("Exporting GPIO pins\n");
	FILE *fd; //repeatedly used for different files
	char path[MAX_BUF]; //also repeatedly used to store file path string
	int i = 0; //general use counter
	for(i=0; i<NUM_OUT_PINS; i++){
		gpio_export(out_gpio_pins[i]);
	};
	printf("Setting GPIO Direction\n");
	for(i=0; i<NUM_OUT_PINS; i++){
		gpio_set_dir(out_gpio_pins[i], OUTPUT_PIN);
	};
	
	//set up function pointers for button and interrupt events
	set_imu_interrupt_func(&null_func);
	set_start_pressed_func(&null_func);
	set_start_unpressed_func(&null_func);
	set_select_pressed_func(&null_func);
	set_select_unpressed_func(&null_func);
    
	
	//Set up PWM
	//set correct polarity such that 'duty' is time spent HIGH
	printf("Initializing PWM\n");
	i=0;
	for(i=0; i<6; i++){
		strcpy(path, pwm_files[i]);
		strcat(path, "polarity");
		fd = fopen(path, "a");
		fprintf(fd,"%c",'0');
		fflush(fd);
		fclose(fd);
	};
	//set the pwm period in nanoseconds
	set_pwm_period_ns(DEFAULT_PERIOD_NS);
	
	//leave duty open for future writes
	for(i=0; i<6; i++){
		strcpy(path, pwm_files[i]);
		strcat(path, "duty");
		pwm_duty_pointers[i] = fopen(path, "a");
		set_motor(i+1,0); //set motor to free-spin
	};
 
	printf("PWM Initialized\n");
	
	//open encoder file pointers to make sure they work
	for(i=0; i<3; i++){
		strcpy(path, encoder_files[i]);
		encoder_pointers[i] = fopen(path, "r");
		fclose(encoder_pointers[i]);
	};
	printf("eQep Encoders Initialized\n");
	

	
	//  Spektrum RC setup on uart4
/* 	uart4_port = open("/dev/ttyO4", O_RDWR | O_NOCTTY);
	if ((uart4_port) < 0){
		printf("error opening uart4 for Spektrum\n");}
	else{
		printf("Enabling Spektrum Radio Control\n");
		pthread_t spektrum_thread;
		pthread_create(&spektrum_thread, NULL, spektrum_read, (void*) NULL);
	} */
	
	printf("Starting Event Handler\n");
	pthread_t event_thread;
	pthread_create(&event_thread, NULL, read_events, (void*) NULL);
	

	printf("Battery Voltage = %fV\n", getBattVoltage());
	
	printf("Cape Initialized\n");
	printf("Pressing Ctrl-C will exit cleanly\n");
	return 0;
}


int set_motor(int motor, float duty){
	PIN_VALUE a;
	PIN_VALUE b;
	if(state == UNINITIALIZED){
		initialize_cape();
	}

	if(motor>6 || motor<1){
		printf("enter a motor value between 1 and 6\n");
		return -1;
	}
	
	//check that the duty cycle is within +-1
	if (duty>1){
		duty = 1;
	}
	else if(duty<-1){
		duty=-1;
	}

	//switch the direction pins to H-bridge
	if (duty>0){
	 	a=HIGH;
		b=LOW;
	}
	else{
		a=LOW;
		b=HIGH;
		duty=-duty;
	}
	gpio_set_value(out_gpio_pins[(motor-1)*2],a);
	gpio_set_value(out_gpio_pins[(motor-1)*2+1],b);

	fprintf(pwm_duty_pointers[motor-1], "%d", (int)(duty*pwm_period_ns));	
	fflush(pwm_duty_pointers[motor-1]);

	return 0;
}



int set_pwm_period_ns(int period){
	if(period <1){
		printf("please use PWM period >1 (nanoseconds)\n");
		return -1;
	}
	int i=0;
	FILE *fd;
	char path[MAX_BUF];
	for(i=0; i<6; i++){
		strcpy(path, pwm_files[i]);
		strcat(path, "period");
		fd = fopen(path, "a");
		fprintf(fd,"%d", period);
		fflush(fd);
		fclose(fd);
	};
	pwm_period_ns = period;
	return 0;
}

long int get_encoder(int encoder){
	char path[MAX_BUF]; //repeatedly used to store file path string
	if((encoder<1)|(encoder >3)){
		printf("Enter encoder number between 1 & 3\n");
		return -1;
	}
	long int j;
	//rewind(encoder_pointers[encoder-1]);
	strcpy(path, encoder_files[encoder-1]);
	encoder_pointers[encoder-1] = fopen(path, "r");
	fscanf(encoder_pointers[encoder-1], "%li", &j);
	fclose(encoder_pointers[encoder-1]);
	return j;
}


int getStartBTN(){
	unsigned int value;
	gpio_get_value(START_BTN, &value);
	return value;
}

int getSelectBTN(){
	unsigned int value;
	gpio_get_value(SELECT_BTN, &value);
	return value;
}

int setGRN(PIN_VALUE i){
	return gpio_set_value(GRN_LED, i);
}

int setRED(PIN_VALUE i){
	return gpio_set_value(RED_LED, i);
}

float getBattVoltage(){
	int raw_adc;
	FILE *AIN6_fd;
	AIN6_fd = fopen("/sys/devices/ocp.3/helper.18/AIN6", "r");
	if(AIN6_fd < 0){
		printf("error reading adc\n");
		return -1;
	}
	fscanf(AIN6_fd, "%i", &raw_adc);
	fclose(AIN6_fd);
	return (float)raw_adc*11.0/1000.0; // time 11 for the voltage divider, divide by 1000 to go from mv to V
}

void* spektrum_read(void* ptr){
	while (1) {
		tcflush(uart4_port, TCIFLUSH);
 		ret=read(uart4_port, &j, RC_CHANNELS*2+2);
		
		if(j[0]!=0xff)
			printf("Spektrum Read Error\n");
		else{
			int i =0;
			while(i<RC_CHANNELS){
				rc_channels[i] = j[i*2+2]<<8 ^ j[i*2+3];
				i++;
			}
			usleep(7000); //packets arrive ~11ms, sleep for a bit less
		}
	
	}	
	return NULL;
}

void* read_events(void* ptr){
	int fd;
    fd = open("/dev/input/event1", O_RDONLY);
    struct input_event ev;
	
	while (state != EXITING){
        read(fd, &ev, sizeof(struct input_event));
		//printf("type %i key %i state %i\n", ev.type, ev.code, ev.value);
        if(ev.type == 1){ //only new data
			switch(ev.code){
				case 0:
					if(ev.value == 1){
						(*imu_interrupt_func)();
					}
				break;
				//start button
				case 1:
					if(ev.value == 1){
						start_btn_state = 0; //unpressed
						(*start_unpressed_func)();
					}
					else{
						start_btn_state = 1; //pressed
						(*start_pressed_func)();
					}
				break;
				//select button
				case 2:	
					if(ev.value == 1){
						select_btn_state = 0; //unpressed
						(*select_unpressed_func)();
					}
					else{
						select_btn_state = 1; //pressed
						(*select_pressed_func)();
					}
				break;
			}
		}
    }
	return NULL;
}

int spektrum_input_raw(int channel){
	return rc_channels[channel-1];
}

float spektrum_input_scaled(int channel){
	int ch = channel-1;
	float x = (float) (rc_channels[ch] - spektrum_means[ch]);
	x = 2.0*x/(spektrum_maxs[ch]-spektrum_mins[ch]);
	if(fabs(x)>1.5)
		return 0; //something went wrong, return 0 to be safe
	else
		return x;
	
}

void ctrl_c(int signo){
	if (signo == SIGINT){
		printf("\nreceived SIGINT Ctrl-C\n");
		//set_state(EXITING);
		// in case of ctrl-c signal, shut down cleanly
		cleanup_cape();
		exit(EXIT_SUCCESS);
 	}
}

int cleanup_cape(){
	set_state(EXITING); 
	int i;
	for(i=1;i<=6;i++){
		set_motor(i,0);
	}

	printf("\nClosing GPIO\n");
	setGRN(0);
	setRED(0);	

	printf("Closing PWM\n");
	for(i=0; i<6; i++){
		fclose(pwm_duty_pointers[i]);
	};
	
	
	
	printf("\nExiting Cleanly\n");
	return 0;
}


// Program to test Hobby Servo or Brushless Speed controller function

// James Strawson - 2013

#include <robotics_cape.h>
#define LOOP_RATE_HZ 50

int main(){
	initialize_cape();
	// struct neccesary for the nanosleep function
	//typedef struct timespec	timespec;
	//timespec t1, t2, t2minust1, sleepRequest, deltaT;

	/*
	//PWM period is twice loop period so duty updates before next pulse starts
	int period_ns = (1000000000/LOOP_RATE_HZ)*2;
	if (set_pwm_period_ns(period_ns)){
		exit(1);
	}
	printf("period set\n");
	*/
	printf("setting esc to 0\n");
	set_esc(1,0);
	
	sleep(1);
	//Keep Running until program state changes
	do{
		//clock_gettime(CLOCK_MONOTONIC, &t1);  //record the time at the beginning.
		
		
		set_esc(1,.1);
		setGRN(HIGH);
		printf("\rON ");
		fflush(stdout);
		sleep(1);
		
		set_esc(1,0);
		printf("\rOFF");
		fflush(stdout);
		setGRN(LOW);
		sleep(1);
		
	
		//Sleep for the necessary time to maintain 200hz
		// clock_gettime(CLOCK_MONOTONIC, &t2);
		// t2minust1 = diff(t1, t2);
		// sleepRequest = diff(t2minust1, deltaT);
		// nanosleep(&sleepRequest, NULL);
	}while(get_state() != EXITING);
	
	kill_esc();
	cleanup_cape();
	return 0;
}

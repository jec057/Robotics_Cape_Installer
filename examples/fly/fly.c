// Fly
// Work in progress, don't use this code.

// James Strawson - 2013

#include <robotics_cape.h>
#define CONTROL_LOOP_RATE_HZ 150

// struct neccesary for the nanosleep function
typedef struct timespec	timespec;
timespec t1, t2, t2minust1, sleepRequest, deltaT;

timespec diff(timespec start, timespec end)
{
	timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000L+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

//If the user holds start for 2 seconds, the program exits cleanly
int on_start_press(){
	sleep(2);
	if(get_start_button() == HIGH){
		set_state(EXITING);
	}
	return 0;
}


void* control_loop_func(void* ptr){
	deltaT.tv_sec = 0;		// create time struct for control loop
	deltaT.tv_nsec = (int)(1000000000/CONTROL_LOOP_RATE_HZ);
 	do{
		clock_gettime(CLOCK_MONOTONIC, &t1);  //record the time at the beginning.
		
		switch (get_state()){
		case RUNNING:	
			
			set_esc(1,0);
			
			break;
			
		case PAUSED:
			
			break;
			
		default:
			break;
		}
		

		//Sleep for the necessary time to maintain loop frequency
		clock_gettime(CLOCK_MONOTONIC, &t2);
		t2minust1 = diff(t1, t2);
		sleepRequest = diff(t2minust1, deltaT);
		nanosleep(&sleepRequest, NULL);
	}while(get_state() != EXITING);
	
	return NULL;
}


int main(){
	initialize_cape();
	initialize_spektrum();
	set_select_pressed_func(&on_select_press); //hold select for 2 seconds to close program
	setRED(1);
	setGRN(0);
	set_state(PAUSED);
	
	Printf("hello and welcome to the BeagleQuad fly program.\n");
	
	
	printf("\nTurn on your transmitter with throttle down\n");

	/// Begin the control and slow threads 
	pthread_t control_thread, slow_thread;
	pthread_create(&control_thread, NULL, control_loop_func, (void*) NULL);


	while(get_state()!=EXITING){
		sleep(1);
	}

	cleanup_cape();
	return 0;
}

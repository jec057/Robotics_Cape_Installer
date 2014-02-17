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
	int i;
 	do{
		clock_gettime(CLOCK_MONOTONIC, &t1);  //record the time at the beginning.
		
		
		
		switch (get_state()){
		case RUNNING:	
			if(get_rc_channel(6)==1){
				set_state(PAUSED);
				kill_esc();
				setRED(HIGH);
				setGRN(LOW);
				printf("Kill Swith Hit\n");
				break;
			}
			for(i=0;i<4;i++){
				set_esc(i+1,(get_rc_channel(1)+1)/2);
			}		
			break;
			
		case PAUSED:
			if(get_rc_channel(6)==-1){
				printf("kill switch released\n");
				printf("move throttle up and down to rearm\n");
				while(get_rc_channel(1)!=-1){
					usleep(100000);
					if(get_state()==EXITING)
						break;
				}
				while(get_rc_channel(1)!=1){
					usleep(100000);
					if(get_state()==EXITING)
						break;
				}
				while(get_rc_channel(1)!=-1){
					usleep(100000);
					if(get_state()==EXITING)
						break;
				}
				setRED(LOW);
				setGRN(HIGH);
				set_state(RUNNING);
				printf("armed and RUNNING\n");
			}
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
	set_start_pressed_func(&on_start_press); //hold select for 2 seconds to close program
	setRED(1);
	setGRN(0);
	set_state(PAUSED);
	int i;
	
	printf("hello and welcome to the BeagleQuad fly program.\n");
	printf("\nTurn on your transmitter with throttle DOWN and kill switch UP\n");
	
	//wait for radio connection and then for throttle off
	while(get_rc_new_flag()==0){
		usleep(100000);
		if(get_state()==EXITING)
			break;
	}
	while(get_rc_channel(1)!=-1){
		usleep(100000);
		if(get_state()==EXITING)
			break;
	}
	while(get_rc_channel(6)!=-1){
		usleep(100000);
		if(get_state()==EXITING)
			break;
	}
	
	printf("now go full throttle and down again to arm\n");
	while(get_rc_channel(1)!=1){
		usleep(100000);
		if(get_state()==EXITING)
			break;
	}
	while(get_rc_channel(1)!=-1){
		usleep(100000);
		if(get_state()==EXITING)
			break;
	}
	
	printf("ARMED!!\n");
	set_state(RUNNING);
	setRED(LOW);
	setGRN(HIGH);
	
	

	/// Begin the control thread
	pthread_t control_thread;
	pthread_create(&control_thread, NULL, control_loop_func, (void*) NULL);

	//check for radio disconnect
	while(get_state()!=EXITING){
		usleep(100000);
	}

	cleanup_cape();
	return 0;
}

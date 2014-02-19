// Fly
// Work in progress, don't use this code.

// James Strawson - 2013

#include <robotics_cape.h>

#define CONTROL_HZ 150	//rate in Hz
#define DT 1/150   		//timestep seconds
#define	SATE_LEN 32		//number of timesteps to retain data
#define TIP_THRESHOLD 0.5

// Functions
int on_start_press();
void* flight_control_loop(void* ptr);
void* io_loop(void* ptr);

//Global Variables
float roll[SATE_LEN], pitch[SATE_LEN], yaw[SATE_LEN];
mpudata_t mpu;

int main(){
	//Initialize
	initialize_cape();
	initialize_spektrum();
	set_start_pressed_func(&on_start_press); //hold start for 2 seconds to close program
	if(initialize_imu(CONTROL_HZ)){
		return -1;
	}
	setRED(1);
	setGRN(0);
	set_state(PAUSED);
	printf("hello and welcome to the BeagleQuad fly program.\n");

	/// Begin the threads!
	pthread_t control_thread, io_thread;
	pthread_create(&control_thread, NULL, flight_control_loop, (void*) NULL);
	pthread_create(&io_thread, NULL, io_loop, (void*) NULL);

	//chill in my crib yo
	while(get_state()!=EXITING){
		usleep(100000);
	}
	//cleanup time
	sleep(1);
	cleanup_cape();
	return 0;
}

//If the user holds start for 2 seconds, the program exits cleanly
int on_start_press(){
	sleep(2);
	if(get_start_button() == HIGH){
		set_state(EXITING);
	}
	return 0;
}

// Flight Control Loop //
void* flight_control_loop(void* ptr){
	timespec beginTime, endTime, executionTime, sleepRequest, deltaT;
	deltaT.tv_sec = 0;	// create time struct for control loop
	deltaT.tv_nsec = (int)(1000000000/CONTROL_HZ);
	memset(&mpu, 0, sizeof(mpudata_t));
	int i;
	
	
	do{
		clock_gettime(CLOCK_MONOTONIC, &beginTime);  //record the time at the beginning.
		//check for kill switch
		if(get_rc_channel(6)==1){
			set_state(PAUSED);
			set_all_esc(0);
			setRED(HIGH);
			setGRN(LOW);
			printf("Kill Swith Hit\n");
			break;
		}
		
		mpu9150_read(&mpu);

		//update state
		roll[0]  = mpu.fusedEuler[VEC3_X]; 
		pitch[0] = mpu.fusedEuler[VEC3_Y];
		yaw[0]  = mpu.fusedEuler[VEC3_Z];
		
		switch (get_state()){
		case RUNNING:	
			
			for(i=0;i<4;i++){
				set_esc(i+1,(get_rc_channel(1)+1)/2);
			}		
			break;
			
		case PAUSED:
			
			break;
			
		default:
			break;
		}
		//Sleep for the necessary time to maintain loop frequency
		clock_gettime(CLOCK_MONOTONIC, &endTime);
		executionTime = diff(beginTime, endTime);
		sleepRequest = diff(executionTime, deltaT);
		nanosleep(&sleepRequest, NULL);
	}while(get_state() != EXITING);
	return NULL;
}

void* io_loop(void* ptr){
	do{
		switch (get_state()){
		case RUNNING:	
			// detect a tip-over
			if(abs(roll[0])>TIP_THRESHOLD || abs(roll[0])>TIP_THRESHOLD){
				set_state(PAUSED);
				set_all_esc(0);
				setRED(HIGH);
				setGRN(LOW);
			}
			printf("\rp%0.2f r%0.2f y%0.2f", pitch[0], roll[0], yaw[0]);
			printf("things");
			fflush(stdout);		
			break;
			
		case PAUSED:
			printf("\nTurn on your transmitter kill switch UP\n");
			printf("Move throttle UP then DOWN to arm\n");
			while(get_rc_new_flag()==0){ //wait for radio connection
				usleep(100000);
				if(get_state()==EXITING)
					break;}
				while(get_rc_channel(6)!=-1){ //wait for kill switch up
					usleep(100000);
					if(get_state()==EXITING)
						return 0;}
				while(get_rc_channel(1)!=-1){ //wait for throttle down
					usleep(100000);
					if(get_state()==EXITING)
						break;}
				while(get_rc_channel(1)!=1){ //wait for throttle up
					usleep(100000);
					if(get_state()==EXITING)
						break;}
				while(get_rc_channel(1)!=-1){ //wait for throttle down
					usleep(100000);
					if(get_state()==EXITING)
					break;}
	
			printf("ARMED!!\n\n");
			set_state(RUNNING);
			setRED(LOW);
			setGRN(HIGH);
			break;
			
		default:
			break;
		}
		usleep(100000); //check buttons at roughly 10 hz,not very accurate)
	}while(get_state() != EXITING);
	return NULL;
}
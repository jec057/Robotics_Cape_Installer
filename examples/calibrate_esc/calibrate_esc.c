// Bare Minimum Skeleton for Robotics Cape Project
// James Strawson - 2013

#include <robotics_cape.h>

int main(){
	initialize_cape();
	
	set_esc(1,0);
	
	printf("make sure the 3-pin PWM connector from each ESCs is connected\n");
	printf("to the cape.\n");
	printf("Before continuing, disconnect propellers or gears from your\n");
	printf("brushless motors and disconnect power from the ESCs\n");
	printf("\n");
	printf("to continue, press the cape START button then apply power to the ESCs\n");
	
	
	while(get_start_button() == LOW){
		usleep(10000);
	}
	setGRN(HIGH);
	//set everything to full throttle to define upper bound
	int i;
	for(i=1; i<=6; i++){
		set_esc(i,1);
	}
	sleep(1);
	
	
	printf("\nstarting calibration, apply power to ESCs\n");
	printf("then press start again to complete calibration\n");
	//printf("you should hear some more beeps then silence\n");
	
	while(get_start_button() == LOW){
		usleep(10000);
	}
	setGRN(LOW);
	//set everything to 0 throttle to define lower bound
	for(i=1; i<=6; i++){
		set_esc(i,0);
	}
	
	sleep(3);
	//Keep Running until program state changes
	
	do{
		//clock_gettime(CLOCK_MONOTONIC, &t1);  //record the time at the beginning.
		
		for(i=1; i<=6; i++){
			set_esc(i,.1);
		}
		setGRN(HIGH);
		printf("\rON ");
		fflush(stdout);
		sleep(1);
		
		for(i=1; i<=6; i++){
			set_esc(i,0);
		}
		printf("\rOFF");
		fflush(stdout);
		setGRN(LOW);
		sleep(1);
		
	}while(get_state() != EXITING);
	
	
	kill_esc();
	cleanup_cape();
	return 0;
}
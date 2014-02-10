#include <robotics_cape.h>



int main(){
	initialize_spektrum();
	int i;
	while(get_state()!=EXITING){
		for(i=0;i<RC_CHANNELS;i++){
			printf("ch%d %f  ", i+1, get_rc_channel(i+1));
		}
		usleep(100000);
		printf("\r");
	}
	return 0;
}

 #include <stdio.h>
 #include <stdlib.h>
 #include <time.h>
 #include <string.h>
 #include <fcntl.h>
 #include <unistd.h>
// #include <getopt.h>
#include <termios.h>
#include <signal.h>	//capture ctrl-c
//#include <sys/ioctl.h>
#include <pthread.h>

#define RC_CHANNELS 9
int rc_channels[RC_CHANNELS];
int tty4_fd;
int rc_new_flag = 0;


const char *byte_to_binary(int x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}
void cleanup(int signo){
	if (signo == SIGINT){
		printf("closing tty4\n");
		close(tty4_fd);
		exit(1);
 	}
}

void* spektrum_checker(void *ptr){
	/* int i=-1;
	char j[2];
	while(1){
		read(tty4_fd, &j, 1);
		//detect end of packet
		if(j[0]==0xFF){
			i=0;
			//usleep(10000);
			//tcflush(tty4_fd, TCIFLUSH);
			//printf("\r");
			printf(byte_to_binary(j[0]));
			printf("\n");
			
		}
		else if (i>30){
			printf("error, end packet not found\n");
			i=-1;
		}
		else if(i>=0){ //read 2 bytes for each channel
			//rc_channels[i] = j[0]<<8 ^ j[1];
			// printf("%d ",rc_channels[i]);
			printf(byte_to_binary(j[0]));
			printf(" ");
			//printf(byte_to_binary(j[1]));
			// printf("    ");
			//printf("%d ", i);
			i++;
		}
	} */
}

int main()
{
	signal(SIGINT, cleanup);	//signal catcher calls cleanup function on exit
	
	struct termios config;
	//memset(&config,0,sizeof(config));
	config.c_iflag &= ~(BRKINT | ICRNL | IGNPAR);
	config.c_iflag=0;
    config.c_oflag=0;
    config.c_cflag= CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more info
    config.c_lflag=0;
    config.c_cc[VTIME]=5;
	if ((tty4_fd = open ("/dev/ttyO4", O_RDWR | O_NOCTTY)) < 0) {
		printf("error opening uart4\n");
	}
	if(cfsetispeed(&config, B115200) < 0) {
		printf("cannot set uart4 baud rate\n");
		return -1;
	}
	if(tcsetattr(tty4_fd, TCSAFLUSH, &config) < 0) { 
		printf("cannot set uart4 attributes\n");
		return -1;
	}  
	
	pthread_t spektrum_thread;
	pthread_create(&spektrum_thread, NULL, spektrum_checker, (void*) NULL);
	
	/* /////////////////////////////////////////////////////////////////
	//everything in here should be in above thread
	int i=-1;
	char j[2];
	
	while(1){
		read(tty4_fd, &j, 2);
		//detect end of packet
		if(j[0]==0xFF && j[1] == 0xFF){
			i=0;
			printf("\r");
			printf("\n");
			
			usleep(5000);
			tcflush(tty4_fd, TCIFLUSH);
			
		}
		else if (i>30){
			printf("error, end packet not found\n");
			i=-1;
		}
		else if(i>=0){ //read 2 bytes for each channel
			rc_channels[i] = j[0]<<8 ^ j[1];
			//printf("%d ",rc_channels[i]);
			printf("%d ", i);
			i++;
		}
	}
	///////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////// */
	int i=-1;
	char j[2];
	while(1){
		read(tty4_fd, &j, 1);
		//detect end of packet
		if(j[0]==0xFF){
			read(tty4_fd, &j, 1);
			if (j[0]==1){
				i=0;
			}
			
			//usleep(10000);
			//tcflush(tty4_fd, TCIFLUSH);
			//printf("\r");
			printf("\n");
			printf(byte_to_binary(j[0]));
			
			
		}
		else if (i>30){
			printf("error, end packet not found\n");
			i=-1;
		}
		else if(i>=0){ //read 2 bytes for each channel
			//rc_channels[i] = j[0]<<8 ^ j[1];
			// printf("%d ",rc_channels[i]);
			printf(byte_to_binary(j[0]));
			printf(" ");
			//printf(byte_to_binary(j[1]));
			// printf("    ");
			//printf("%d ", i);
			i++;
		}
	}
	
	close(tty4_fd);
	return 0;
}
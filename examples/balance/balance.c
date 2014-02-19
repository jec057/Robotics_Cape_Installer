// BeagleMIP Balance - James Strawson 2013

#include <robotics_cape.h>
#include "c_i2c.h"
#include "MPU6050.h"

#define DT 0.005       	//5ms loop (200hz)
#define WHEEL_RADIUS 0.035  // meters
#define TRACK_WIDTH 0.1 	//meters, width between contact patches
#define PI 3.14159265358

#define LEAN_THRESHOLD 0.6  //radians lean before killing motors
#define THETA_REF_MAX 0.5	// Maximum reference theta set point for inner loop
#define START_THRESHOLD 0.2 // how close to vertical before it will start balancing

// complementary high and low pass filter constants, plus integrator trim
#define THETA_MIX_TC  3   // t_seconds timeconstant on filter
const float HP_CONST = THETA_MIX_TC/(THETA_MIX_TC + DT);
const float LP_CONST = DT/(THETA_MIX_TC + DT);
float thetaTrim = 0;

// i2c declarations
static i2c_t MPU6050;
static i2c_t* pi2c = &MPU6050;

// Estimator variables
float xAccel, zAccel, yGyro, yGyroOffset, accLP, gyroHP, theta;

//enocder Variables
long int encoderCountsL, encoderCountsR;
long int encoderOffsetL, encoderOffsetR;

///Controller & State Variables ///
float prescaler = 0.7;
float theta, phi[2];
float phiRef; //system state
float oldTheta;

float eTheta[3];
float ePhi[2];
float u[3];
float thetaRef[2];

//Steering controller
float Gamma[2];
float gammaRef, torqueSplit;
float kTurn = 1.0;
float dutyLeft, dutyRight;

// Control constants
float numD1[] = {-6.0977, 11.6581, -5.5721};
float denD1[] = {1.0000,   -1.6663,    0.6663};
float numD2[] = {0.0987,   -0.0985};
float denD2[] = {1.0000,   -0.9719};
float kTrim = -0.1;  // outer loop integrator constant
float kInner = 1.8; //
float kOuter = 1.6; //


// struct neccesary for the nanosleep function
typedef struct timespec	timespec;
timespec t1, t2, t2minust1, sleepRequest, deltaT;



//////////////////////////
// Function Definitions //
//////////////////////////

// gives the difference between two timespecs
// used to calcualte how long to sleep for
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

// start I2C communication with MPU-9150/6050
void i2cStart(){
	//printf("MPU6050 test\n\n");
	pi2c->bus = 1;
	i2c_init(pi2c, pi2c->bus, 0x68);
	openConnection(pi2c);
	bool mpu6050TestResult = MPU_testConnection(pi2c, pi2c->buffer);
	if(mpu6050TestResult) {
		printf("MPU6050 test passed \n");
	} else {
		printf("MPU6050 test failed \n");
	}
}

//returns theta after a fresh configuration and warmup
float initializeEstimator(){
	int16_t ax, ay, az;
	int16_t gx, gy, gz;
	int i = 0;
	float xAccelCount, zAccelCount, yGyroCount;
	uint8_t buffer[14];
	
	MPU_setSleepEnabled(false, pi2c);  			  	// Wake up from sleep
	MPU_setClockSource(MPU6050_CLOCK_PLL_XGYRO, pi2c);		// setup MPU6050	
	MPU_setDLPFMode(0, pi2c); 					// as little filtering as possible	
	MPU_setFullScaleGyroRange(MPU6050_GYRO_FS_1000, pi2c); 	// GYRO_FS_1000  +-1000 deg/s range	
	MPU_setFullScaleAccelRange(MPU6050_ACCEL_FS_2, pi2c);	// MPU6050_ACCEL_FS_2    +- 2g range

	usleep(10000); // let the gyro settle

	// warm up loop, sample data 100 times
	for (i = 0; i < 100; i++){ 
		MPU_getMotion6(&ax, &ay, &az, &gx, &gy, &gz, pi2c, buffer); 
		xAccelCount += (float)ax; 
		zAccelCount += (float)az;
		yGyroCount += (float)gy;
		usleep(5000);
		//printf("gy: %f  ax: %f  az: %f\n", (float)gy, (float)ax, (float)ay);            
	}
		
	yGyroOffset = yGyroCount/100;		//offset to correct for steady-state gyro error
    	accLP = -atan2(zAccelCount, -xAccelCount); 		//initialize accLP so filter begins at correct theta
	theta=accLP;
	printf("yGyroOffset = %f\n", yGyroOffset);
	printf("Theta = %f\n", theta);
	return accLP;
}


//////////////////////////////////////////
// Complementary Filter		   //
// Returns the latest value for theta.  //
// You must call this function at 200hz //
//////////////////////////////////////////

float Complementary_Filter(){
	int16_t ax, ay, az;
	int16_t gx, gy, gz;

	MPU_getMotion6(&ax, &ay, &az, &gx, &gy, &gz, pi2c, pi2c->buffer); 
      
	// Apply preScale for +-2g range
	xAccel = (float)ax*2/32768;  
	zAccel = (float)az*2/32768; 

	//subtract the steady-state gyro offset error
	//then multiply by the gyro prescaler
	yGyro = (gy-yGyroOffset)*1000*2*PI/(360*32768);            

	//first order filters
	accLP = accLP + LP_CONST * (-atan2(zAccel,-xAccel) - accLP);
	gyroHP = HP_CONST*(gyroHP + .005*yGyro);

	// diagnostic print to console
	//printf("gyroHP: %f accelLP: %f theta: %f Phi: %f\n", gyroHP, accLP, gyroHP+accLP, phi); 

	return (gyroHP+accLP)+thetaTrim;
}

//If the user holds select for 2 seconds, the program exits cleanly
int on_select_press(){
	sleep(2);
	if(get_select_button() == HIGH){
		set_state(EXITING);
	}
	return 0;
}


///////////////////////////////////////////////////////
// 10hz Loop checking battery, buttons, and tipover ///
///////////////////////////////////////////////////////
void* slow_loop_func(void* ptr){
	do{
		switch (get_state()){
		case RUNNING:	
			// detect a tip-over
			if(fabs(theta)>LEAN_THRESHOLD){
				set_state(PAUSED);
				set_motor(1,0);
				set_motor(3,0);
				setRED(HIGH);
				setGRN(LOW);
			}
			break;
			
		case PAUSED:
			if(fabs(theta)<START_THRESHOLD){
				setGRN(HIGH); //tell user it's upright enough to start
				sleep(1); //wait a second before starting balancing
				if(fabs(theta)<START_THRESHOLD){
					set_state(RUNNING);
					setRED(LOW);
				}
				else{
					setGRN(LOW);
				}
			}
			break;
			
		default:
			break;
		}
		
		
		//printf("\r                                                             ");
		printf("\rtheta: %0.2f phi: %0.2f gamma: %0.2f ", theta, phi[0], Gamma[0]);
		printf("u[0]: %0.2f  ", u[0]);
		fflush(stdout);
		usleep(100000); //check buttons at roughly 10 hz,not very accurate)
	}while(get_state() != EXITING);
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
/// running 200hz discrete estimator and controller in asychronous loop //
//////////////////////////////////////////////////////////////////////////
void* control_loop_func(void* ptr){
 	do{
		clock_gettime(CLOCK_MONOTONIC, &t1);  //record the time at the beginning.
		
	
		encoderCountsL = (get_encoder(1)-encoderOffsetL);
		encoderCountsR = -(get_encoder(2)-encoderOffsetR);
		phi[1]=phi[0];
		phi[0] = (encoderCountsL+encoderCountsR)*PI/352; //convert to radians, 352 ticks per revolution
		Gamma[1]=Gamma[0];
		Gamma[0]= WHEEL_RADIUS*2*(encoderCountsL-encoderCountsR)*PI/(352*TRACK_WIDTH);
		theta = Complementary_Filter();

	
		switch (get_state()){
		case RUNNING:	
			// step difference equation forward
			ePhi[1]=ePhi[0];
			thetaRef[1]=thetaRef[0];
			eTheta[2]=eTheta[1]; eTheta[1]=eTheta[0];
			u[2]=u[1]; u[1]=u[0];
  
			ePhi[0] = phiRef-phi[0];
			thetaRef[0] = kOuter*(numD2[0]*ePhi[0] + numD2[1]*ePhi[1]) - denD2[1]*thetaRef[1];
			
			//integrate the reference theta to correct for imbalance or sensor error
			thetaTrim += kTrim * thetaRef[0]*DT;

			if(thetaRef[0]>THETA_REF_MAX){thetaRef[0]=THETA_REF_MAX;}
			else if(thetaRef[0]<-THETA_REF_MAX){thetaRef[0]=-THETA_REF_MAX;}

			eTheta[0] = (prescaler * thetaRef[0]) - theta;
			u[0] = kInner*(numD1[0]*eTheta[0]+numD1[1]*eTheta[1] + numD1[2]*eTheta[2]) - denD1[1]*u[1] - denD1[2]*u[2]; 
			
			if(u[0]>1){
				u[0]=1;
			}
			else if(u[0]<-1){
				u[0]=-1;
			}
   
			torqueSplit = kTurn*(gammaRef - Gamma[0]);
			dutyLeft = u[0]-torqueSplit;
			dutyRight = u[0]+torqueSplit;			
			set_motor(1,-dutyRight);
			set_motor(3,dutyLeft); //minus because motor is flipped on chassis
			break;
		case PAUSED:
			encoderOffsetL = get_encoder(1);
			encoderOffsetR = get_encoder(2);
			ePhi[1]=0; ePhi[0]=0;
			thetaRef[1]=0; thetaRef[0]=0;
			eTheta[2]=0; eTheta[1]=0; eTheta[0]=0;
			u[2]=0; u[1]=0; u[0]=0;
			break;
		default:
			break;
		}
		

		//Sleep for the necessary time to maintain 200hz
		clock_gettime(CLOCK_MONOTONIC, &t2);
		t2minust1 = diff(t1, t2);
		sleepRequest = diff(t2minust1, deltaT);
		nanosleep(&sleepRequest, NULL);
	}while(get_state() != EXITING);
	return NULL;
}


//////////////////////////////////////////////////////////////////
/// Main function used for initializtion and thread management ///
//////////////////////////////////////////////////////////////////

int main(){
	initialize_cape();
	
	deltaT.tv_sec = 0;		// create time struct for 5ms (200hz)
	deltaT.tv_nsec = 5000000;
	set_select_pressed_func(&on_select_press); //hold select for 2 seconds to close program
	i2cStart();
	initializeEstimator();
	setRED(1);
	setGRN(0);
	
	printf("\nHold your MIP upright to begin balancing\n");

	/// Begin the control and slow threads 
	pthread_t control_thread, slow_thread;
	pthread_create(&control_thread, NULL, control_loop_func, (void*) NULL);
	pthread_create(&slow_thread, NULL, slow_loop_func, (void*) NULL);
	struct sched_param params;
	// We'll set the priority to the maximum.
	params.sched_priority = sched_get_priority_max(SCHED_FIFO);
	if (pthread_setschedparam(control_thread, SCHED_FIFO, &params)) {
		printf("Unsuccessful in setting thread realtime prio\n");
		return -1;
	}
	//pthread_join(control_thread, NULL);
	//pthread_join(slow_thread, NULL);

	while(get_state()!=EXITING){
		sleep(1);
	}

	//nothing get executed here until runTrue == 0 which terminates the threads
	//usleep(100000); //let other processes clean up
	closeConnection(pi2c);
	cleanup_cape();
	return 0;
}
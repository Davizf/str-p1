/**********************************************************
 *  INCLUDES
 *********************************************************/
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ioLib.h>
#include "displayA.h"
#include "serialcallLib.h"
#include <time.h>

/**********************************************************
 *  Constants
 **********************************************************/
#define SIMULATOR 1// 1 use simulator, 0 use serial
#define SECURE_SPEED 55
#define BRIGHTNESS_ON 50

#define NS_PER_S  1000000000
#define PERIOD_NS  500000000
#define TIME_TO_RELOAD   30
#define INIT_HIGH   0

// DISPLAY DELAY
#define TIME_DISPLAY_SEC 0
#define TIME_DISPLAY_NSEC 500000000

// FORCING ERRORS
#define TIME_BW_ERRORS   30
#define ERROR_DELAY_TIME 1

/**********************************************************
 *  Global Variables
 *********************************************************/
float speed = 0.0;
int gasState = 0;
int brakeState = 0;
int mixState = 0;
int slope = 0;
int brightness = 0;
int lights = 0;
struct timespec mix_start, mix_end, mix_aux;

int SECONDARY_CYCLES=6;
struct timespec SC_DURATION={.tv_sec=5, .tv_nsec=0};// Max duration of an CS


//---------------------------------------------------------------------------
//                           FUNCIONES AUXILIARES
//---------------------------------------------------------------------------
/**********************************************************
 *  Function: getClock
 *********************************************************/
double getClock()
{
    struct timespec tp;
    double reloj;

    clock_gettime (CLOCK_REALTIME, &tp);
    reloj = ((double)tp.tv_sec) +
	    ((double)tp.tv_nsec) / ((double)NS_PER_S);
    //printf ("%d:%d",tp.tv_sec,tp.tv_nsec);

    return (reloj);
}

/**********************************************************
 *  Function: diffTime
 *********************************************************/
void diffTime(struct timespec end,
			  struct timespec start,
			  struct timespec *diff)
{
	if (end.tv_nsec < start.tv_nsec) {
		diff->tv_nsec = NS_PER_S - start.tv_nsec + end.tv_nsec;
		diff->tv_sec = end.tv_sec - (start.tv_sec+1);
	} else {
		diff->tv_nsec = end.tv_nsec - start.tv_nsec;
		diff->tv_sec = end.tv_sec - start.tv_sec;
	}
}

/**********************************************************
 *  Function: addTime
 *********************************************************/
void addTime(struct timespec end,
			  struct timespec start,
			  struct timespec *add)
{
	unsigned long aux;
	aux = start.tv_nsec + end.tv_nsec;
	add->tv_sec = start.tv_sec + end.tv_sec +
			      (aux / NS_PER_S);
	add->tv_nsec = aux % NS_PER_S;
}

/**********************************************************
 *  Function: task_speed
 *********************************************************/
int task_speed()
{
	char request[10];
	char answer[10];

	//--------------------------------
	//  request speed and display it
	//--------------------------------

	//clear request and answer
	memset(request,'\0',10);
	memset(answer,'\0',10);

	// request speed
	strcpy(request,"SPD: REQ\n");

	if (SIMULATOR) {
		simulator(request, answer);
	} else {
		writeSerialMod_9(request);
		readSerialMod_9(answer);
	}

	// display speed
	if (1 == sscanf (answer,"SPD:%f\n",&speed)) {
		displaySpeed(speed);
	}
	return 0;
}

/**********************************************************
 *  Function: task_slope
 *********************************************************/
int task_slope()
{
	char request[10];
	char answer[10];

	//--------------------------------
	//  request slope and display it
	//--------------------------------

	//clear request and answer
	memset(request,'\0',10);
	memset(answer,'\0',10);

	// request slope
	strcpy(request,"SLP: REQ\n");

	if (SIMULATOR) {
		simulator(request, answer);
	} else {
		writeSerialMod_9(request);
		readSerialMod_9(answer);
	}

	// display slope
	if (0 == strcmp(answer,"SLP:DOWN\n")) {
		slope = -1;
		displaySlope(-1);
	} else if (0 == strcmp(answer,"SLP:FLAT\n")) {
		slope = 0;
		displaySlope(0);
	} else if (0 == strcmp(answer,"SLP:  UP\n")) {
		slope = 1;
		displaySlope(1);
	}

	return 0;
}


/**********************************************************
 *  Function: task_gas
 *********************************************************/
int task_gas()
{
	char request[10];
	char answer[10];

	//----------------------------------------------------------
	//  check the actual speed and decide whether to accelerate
	//----------------------------------------------------------

	//clear request and answer
	memset(request,'\0',10);
	memset(answer,'\0',10);

	// accelerate if the speed is lower than 55m/s
	if(speed >= SECURE_SPEED && gasState == 1){
		strcpy(request,"GAS: CLR\n");
		gasState = 0;
	}else if(speed < SECURE_SPEED && gasState == 0){
		strcpy(request,"GAS: SET\n");
		gasState = 1;
	}else{
		// other cases do nothing
		return 0;
	}

	if (SIMULATOR) {
		simulator(request, answer);
	} else {
		writeSerialMod_9(request);
		readSerialMod_9(answer);
	}

	// display speed
	if (0 == strcmp(answer,"GAS:  OK\n")) {
		displayGas(gasState);
		return 0;
	}

	// error case
	return -1;
}

/**********************************************************
 *  Function: task_brake
 *********************************************************/
int task_brake()
{
	char request[10];
	char answer[10];

	//----------------------------------------------------------
	//  check the actual speed and decide whether to decelerate
	//----------------------------------------------------------

	//clear request and answer
	memset(request,'\0',10);
	memset(answer,'\0',10);

	// decelerate if the speed is more than 55m/s
	if(speed <= SECURE_SPEED && brakeState == 1){
		strcpy(request,"BRK: CLR\n");
		brakeState = 0;
	}else if(speed > SECURE_SPEED && brakeState == 0){
		strcpy(request,"BRK: SET\n");
		brakeState = 1;
	}else{
		// other cases do nothing
		return 0;
	}

	if (SIMULATOR) {
		simulator(request, answer);
	} else {
		writeSerialMod_9(request);
		readSerialMod_9(answer);
	}

	// display speed
	if (0 == strcmp(answer,"BRK:  OK\n")) {
		displayBrake(brakeState);
		return 0;
	}

	// error case
	return -1;
}

/**********************************************************
 *  Function: task_mix
 *********************************************************/
int task_mix()
{
	double diff_t;
	char request[10];
	char answer[10];

	//-------------------------------------------------
	//  checking time count and decide to activate mix
	//-------------------------------------------------

	clock_gettime(CLOCK_REALTIME, &mix_end);

	diffTime(mix_end, mix_start, &mix_aux);
	if(mix_aux.tv_sec < 30){
		return 0;
	}

	//clear request and answer
	memset(request,'\0',10);
	memset(answer,'\0',10);

	if(mixState == 0){
		mixState = 1;
		// request speed
		strcpy(request,"MIX: SET\n");
	}else{
		mixState = 0;
		strcpy(request,"MIX: CLR\n");
	}

	if (SIMULATOR) {
		simulator(request, answer);
	} else {
		writeSerialMod_9(request);
		readSerialMod_9(answer);
	}
	//restart the counting
	clock_gettime(CLOCK_REALTIME, &mix_start);

	// display speed
	if (0 == strcmp (answer,"MIX:  OK\n")) {
		displayMix(mixState);
		return 0;
	}

	// error case
	return -1;
}

/**********************************************************
 *  Function: task_brightness
 *********************************************************/
int task_brightness()
{
	char request[10];
	char answer[10];

	//--------------------------------
	//  request brightness and display it
	//--------------------------------

	//clear request and answer
	memset(request,'\0',10);
	memset(answer,'\0',10);

	// request brightness
	strcpy(request,"LIT: REQ\n");

	if (SIMULATOR) {
		simulator(request, answer);
	} else {
		writeSerialMod_9(request);
		readSerialMod_9(answer);
	}

	// display brightness
	if (1 == sscanf (answer,"LIT: %d%%\n",&brightness)) {
		displayLightSensor(brightness);
	}
	return 0;
}

/**********************************************************
 *  Function: task_lights
 *********************************************************/
int task_lights()
{
	char request[10];
	char answer[10];

	//clear request and answer
	memset(request,'\0',10);
	memset(answer,'\0',10);

	// Turn ON if brightness < 50%
	if(brightness < BRIGHTNESS_ON && lights == 0){
		strcpy(request,"LAM: SET\n");
		lights = 1;
	}else if(brightness >= BRIGHTNESS_ON && lights == 1){
		strcpy(request,"LAM: CLR\n");
		lights = 0;
	}else{
		// other cases do nothing
		return 0;
	}

	if (SIMULATOR) {
		simulator(request, answer);
	} else {
		writeSerialMod_9(request);
		readSerialMod_9(answer);
	}

	// display speed
	if (0 == strcmp(answer,"LAM:  OK\n")) {
		displayLamps(lights);
		return 0;
	}

	// error case
	return -1;
}


/**********************************************************
 *  Function: controller
 *********************************************************/
void *controller(void *arg)
{
	int actual_sc=0;
	struct timespec start, finish, sleep;// Cuando empieza y acaba el CS
	clock_gettime(CLOCK_REALTIME, &start);

	// init counting time for mix
	clock_gettime(CLOCK_REALTIME, &mix_start);

	// Endless loop
	while(1) {
		switch (actual_sc) {
		case 0:
			task_slope();// A
			task_speed();// B
			task_lights();// F
			task_brightness();// G
			task_mix();// E
			break;
		case 1:
			task_gas();// C
			task_brake();// D
			task_lights();// F
			task_brightness();// G
			break;
		case 2:
			task_slope();// A
			task_speed();// B
			task_lights();// F
			task_brightness();// G
			break;
		case 3:
			task_gas();// C
			task_brake();// D
			task_lights();// F
			task_brightness();// G
			task_mix();// E
			break;
		case 4:
			task_slope();// A
			task_speed();// B
			task_lights();// F
			task_brightness();// G
			break;
		case 5:
			task_gas();// C
			task_brake();// D
			task_lights();// F
			task_brightness();// G
			break;
		}
		actual_sc=(actual_sc+1)%SECONDARY_CYCLES;

		clock_gettime(CLOCK_REALTIME, &finish);

		// sleep = (start + SC_DURATION) - finish
		addTime(start, SC_DURATION, &start);
		diffTime(start, finish, &sleep);

		clock_nanosleep(CLOCK_REALTIME, 0, &sleep, NULL);
	}
}

/**********************************************************
 *  Function: main
 *********************************************************/
int main ()
{
	pthread_t thread_ctrl;
	sigset_t alarm_sig;
	int i;
	
	/* Block all real time signals so they can be used for the timers.
	   Note: this has to be done in main() before any threads are created
	   so they all inherit the same mask. Doing it later is subject to
	   race conditions */
	sigemptyset (&alarm_sig);
	for (i = SIGRTMIN; i <= SIGRTMAX; i++) {
		sigaddset (&alarm_sig, i);
	}
	sigprocmask (SIG_BLOCK, &alarm_sig, NULL);

	// init display
	displayInit(SIGRTMAX);
	if (SIMULATOR == 0) {
		initSerialMod_WIN_9600 ();
	}

	/* Create first thread */
	pthread_create (&thread_ctrl, NULL, controller, NULL);
	pthread_join (thread_ctrl, NULL);
	return (0);
}

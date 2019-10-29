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
#define secureSpeed 55
#define secureSpeedMinimum 40
#define secureSpeedMaximum 70

/**********************************************************
 *  Global Variables
 *********************************************************/
float speed = 0.0;
int gasState = 0;
int brakeState = 0;
int mixState = 0;
time_t mix_start_t, mix_end_t;

int SECONDARY_CYCLES=6;
struct timespec SC_DURATION={.tv_sec=5, .tv_nsec=0};// Max duration of an CS

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

	//uncomment to use the simulator
	simulator(request, answer);

	// uncoment to access serial module
	//writeSerialMod_9(request);
	//readSerialMod_9(answer);

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

	//uncomment to use the simulator
	simulator(request, answer);

	// uncoment to access serial module
	//writeSerialMod_9(request);
	//readSerialMod_9(answer);

	// display slope
	if (0 == strcmp(answer,"SLP:DOWN\n")) displaySlope(-1);
	if (0 == strcmp(answer,"SLP:FLAT\n")) displaySlope(0);
	if (0 == strcmp(answer,"SLP:  UP\n")) displaySlope(1);

	return 0;
}

/**********************************************************
 *  Auxiliar function: get_actual_speed
 *********************************************************/
int get_actual_speed()
{
	char request[10];
	char answer[10];

	//--------------------------------
	//  return the actual speed
	//--------------------------------

	//clear request and answer
	memset(request,'\0',10);
	memset(answer,'\0',10);

	// request speed
	strcpy(request,"SPD: REQ\n");

	//uncomment to use the simulator
	simulator(request, answer);

	// display speed
	if (1 == sscanf (answer,"SPD:%f\n",&speed)) {
		return speed;
	}

	// error case
	return -1;
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

	// accelerate if the speed is lower than 40m/s
	if(get_actual_speed() > secureSpeedMinimum && gasState == 0){
		return 0;
	}else if(get_actual_speed() > secureSpeedMinimum && gasState == 1){
		strcpy(request,"GAS: CLR\n");
		gasState = 0;
	}else if(get_actual_speed() < secureSpeedMinimum && gasState == 0){
		strcpy(request,"GAS: SET\n");
		gasState = 1;
	}

	//uncomment to use the simulator
	simulator(request, answer);

	// uncoment to access serial module
	//writeSerialMod_9(request);
	//readSerialMod_9(answer);

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

	// decelerate if the speed is more than 70m/s
	if(get_actual_speed() < secureSpeedMaximum && brakeState == 0){
		return 0;
	}else if(get_actual_speed() < secureSpeedMaximum && brakeState == 1){
		strcpy(request,"BRK: CLR\n");
		brakeState = 0;
	}else if(get_actual_speed() > secureSpeedMaximum && brakeState == 0){
		strcpy(request,"BRK: SET\n");
		brakeState = 1;
	}

	//uncomment to use the simulator
	simulator(request, answer);

	// uncoment to access serial module
	//writeSerialMod_9(request);
	//readSerialMod_9(answer);

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

	time(&mix_end_t);

	if(difftime(mix_end_t, mix_start_t) < 30){
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

	//uncomment to use the simulator
	simulator(request, answer);

	//restart the counting
	time(&mix_start_t);

	// uncoment to access serial module
	//writeSerialMod_9(request);
	//readSerialMod_9(answer);

	// display speed
	if (0 == strcmp (answer,"MIX:  OK\n")) {
		displayMix(mixState);
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

	// Endless loop
	while(1) {
		switch (actual_sc) {
		case 0:
			task_speed();// A
			task_slope();// B
			task_gas();// C
			break;
		case 1:
			task_brake();// D
			task_mix();// E
			break;
		case 2:
			task_speed();// A
			task_slope();// B
			break;
		case 3:
			task_gas();// C
			task_brake();// D
			break;
		case 4:
			task_speed();// A
			task_slope();// B
			task_gas();// C
			break;
		case 5:
			task_brake();// D
			task_mix();// E
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

	// initSerialMod_9600 uncomment to work with serial module
	//initSerialMod_WIN_9600 ();

	// init counting time for mix
	time(&mix_start_t);

	/* Create first thread */
	pthread_create (&thread_ctrl, NULL, controller, NULL);
	pthread_join (thread_ctrl, NULL);
	return (0);
}

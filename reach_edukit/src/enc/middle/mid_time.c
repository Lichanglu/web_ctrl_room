#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include "nslog.h"


unsigned long long getCurrentTime2(void);


static unsigned long long g_start_time = 0;
unsigned long long app_get_start_time()
{
	if(g_start_time == 0) {
		g_start_time = getCurrentTime2();
	}

	return g_start_time;
}
int app_set_start_time()
{
	g_start_time = getCurrentTime2();
	return 0;
}


/*get time */
unsigned long long getCurrentTime_ex(void)
{
	struct timeval tv;
	struct timezone tz;
	unsigned long long ultime = 0 ;

	gettimeofday(&tv , &tz);
	ultime = tv.tv_sec;
	ultime = ultime * 1000 + tv.tv_usec / 1000;

	printf("SHIT SEC : %u  USEC : %u  %u\n", (unsigned int)tv.tv_sec, (unsigned int)tv.tv_usec, (unsigned int)ultime);

	return (ultime);
}


//获取毫秒级别
unsigned int get_cur_time(void)
{
	unsigned int msec;
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	msec = tp.tv_sec;
	msec = msec * 1000 + tp.tv_nsec / 1000000;
	//msec +=0x5520000;
	//msec +=0x2D82D82 -1000*120;
	//printf("mid_clock ultime=%x==%x\n",msec,tp.tv_sec);
	return msec;
}

#if 1
//获取毫秒级别
unsigned int mid_clock(void)
{

	//	unsigned int msec;
	//	struct timeval    tv;

	//	gettimeofday(&tv,NULL);

	//	msec = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	unsigned int msec;
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);

	msec = tp.tv_sec;
	msec = msec * 1000 + tp.tv_nsec / 1000000;

	return msec;
}
#endif

/*get time */
unsigned long long getCurrentTime2(void)
{
	struct timeval tv;
	struct timezone tz;
	unsigned long long ultime;


	gettimeofday(&tv , &tz);
	ultime = tv.tv_sec;
	ultime = ultime * 1000 + tv.tv_usec / 1000;
	//	nslog(NS_INFO,"getimeofday : %d --- %d \n",tv.tv_sec,tv.tv_usec);
	//	ultime = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	return (ultime);
}



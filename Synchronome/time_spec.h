#ifndef _TIME_SPEC_H_
#define _TIME_SPEC_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <semaphore.h>
#include <mqueue.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <errno.h>
#include <string.h>
#include <signal.h>



#define COUNT_TO_LOAD       (33333333)
#define HZ_HALF             (15)
#define HZ_1                (30)
#define HZ_10               (3)
//#define HZ_30               (3)

#define USEC_PER_MSEC       (1000)
#define NANOSEC_PER_MSEC    (1000000.0)
#define NANOSEC_PER_SEC     (1000000000.0)
#define MSEC_PER_SEC        (1000.0)

// Of the available user space clocks, CLOCK_MONONTONIC_RAW is typically most precise and not subject to 
// updates from external timer adjustments
//
// However, some POSIX functions like clock_nanosleep can only use adjusted CLOCK_MONOTONIC or CLOCK_REALTIME
//
//#define MY_CLOCK_TYPE CLOCK_REALTIME
#define MY_CLOCK_TYPE CLOCK_MONOTONIC
//#define MY_CLOCK_TYPE CLOCK_MONOTONIC_RAW
//#define MY_CLOCK_TYPE CLOCK_REALTIME_COARSE
//#define MY_CLOCK_TYPE CLOCK_MONTONIC_COARSE


double getTimeMsec(void);
double realtime(struct timespec *tsptr);
double dTime(struct timespec now, struct timespec start);

#endif // _CAMERA_DRIVER_H_
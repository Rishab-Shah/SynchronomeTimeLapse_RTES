#ifndef _SERVICES_H_
#define _SERVICES_H_

#define _GNU_SOURCE

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

#include "time_spec.h"

#define TRUE                (1)
#define FALSE               (0)

void *Service_1_frame_acquisition(void *threadp);
void *Service_2_frame_process(void *threadp);
void *Service_3_frame_storage(void *threadp);


#endif // _SERVICES_H_
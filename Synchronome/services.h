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
#include "camera_driver.h"

#define TRUE                  (1)
#define FALSE                 (0)

#define SNDRCV_MQ	            ("/send_receive_mq_image")
#define SNDRCV_MQ_2           ("/send_receive_mq_image_2")
#define SNDRCV_MQ_3           ("/send_receive_mq_image_3")

#define MAX_MSG_SIZE	        (100)
#define ERROR		              (-1)

void *Service_0_Sequencer(void *threadp);
void *Service_1_frame_acquisition(void *threadp);
void *Service_2_frame_process(void *threadp);
void *Service_3_transformation_process(void *threadp);
void *Service_4_transformation_on_off(void *threadp);

void *writeback_dump(void *threadp);



#endif // _SERVICES_H_
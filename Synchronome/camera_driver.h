#ifndef _CAMERA_DRIVER_H_
#define _CAMERA_DRIVER_H_

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

enum io_method 
{
  IO_METHOD_READ,
  IO_METHOD_MMAP,
  IO_METHOD_USERPTR,
};

#define START_UP_FRAMES         (15)
#define LAST_FRAMES             (1)
#define WRITEBACK_FRAMES        (1800)
#define CAPTURE_FRAMES          (WRITEBACK_FRAMES + LAST_FRAMES)
#define FRAMES_TO_ACQUIRE       (CAPTURE_FRAMES + START_UP_FRAMES + LAST_FRAMES)
#define BUFF_LENGTH             (CAPTURE_FRAMES)
#define COLOR_CONVERT_RGB       (1)

void errno_exit(const char *s);


#endif // _CAMERA_DRIVER_H_
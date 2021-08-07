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

#define START_UP_FRAMES         (1)
#define LAST_FRAMES             (1)


//for 1 min (10:1)
//#define WRITEBACK_FRAMES        (190)     //18
//#define FRAMES_TO_STORE         (18)

//for 3 mins (10:1)
#define WRITEBACK_FRAMES        (1810)      //180 //s1 made use karto
#define FRAMES_TO_STORE         (180)


// for 30 mins (10:1)
//#define WRITEBACK_FRAMES        (18010)   //1800
//#define FRAMES_TO_STORE         (1800)

// for 30 mins (1:1) 1 hz
//#define WRITEBACK_FRAMES        (1800)     //180 //s1 made use karto
//#define FRAMES_TO_STORE         (1800)


// for 3 mins (1:1) 1 hz
//#define WRITEBACK_FRAMES        (180)     //180 //s1 made use karto
//#define FRAMES_TO_STORE         (180)


// for 3 mins (1:1) 10 hz
//#define WRITEBACK_FRAMES        (1800)     //180 //s1 made use karto
//#define FRAMES_TO_STORE         (1800)


#define CAPTURE_FRAMES          (WRITEBACK_FRAMES + LAST_FRAMES)
#define FRAMES_TO_ACQUIRE       (CAPTURE_FRAMES + START_UP_FRAMES + LAST_FRAMES)
#define BUFF_LENGTH             (CAPTURE_FRAMES)
#define COLOR_CONVERT_RGB       (1)

void errno_exit(const char *s);


#endif // _CAMERA_DRIVER_H_
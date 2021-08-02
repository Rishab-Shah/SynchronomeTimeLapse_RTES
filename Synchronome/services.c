#include "services.h"

extern void *tempptr;
extern unsigned char temp_g_buffer[614400];

extern void *buffptr;
extern unsigned char bigbuffer[(1280*960)];

extern void *buffptr_transform;
extern unsigned char negativebuffer[(1280*960)];

struct mq_attr mq_attr;
mqd_t mymq;

struct mq_attr mq_attr2;
mqd_t mymq2;

struct mq_attr mq_attr3;
mqd_t mymq3;

int sid,rid;

void message_queue_setup();
void message_queue_release();

extern void process_transform(const void *p, int size);
extern void process_image(const void *p, int size);
int process_transform_servicepart(int *frame_stored);

extern void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time);
extern void dump_pgm(const void *p, int size, unsigned int tag, struct timespec *time);

extern int framecnt;
extern struct timespec frame_time;
extern int g_framesize;

// semaphore - start
int abortS1=FALSE, abortS2=FALSE, abortS3=FALSE;
sem_t semS1, semS2, semS3;
int abortTest=FALSE;
timer_t timer_1;
struct itimerspec last_itime;
struct itimerspec itime = {{1,0}, {1,0}};
double start_realtime;
unsigned long long seqCnt=0;
void Sequencer(int id);
// semaphore - stop

extern void mainloop(void);

//timestmap
/* writeback time capturing */
extern struct timespec ts_writeback_start,ts_writeback_stop;
extern double writeback_time[BUFF_LENGTH];
/* end to end time for any service */
extern struct timespec ts_end_to_end_start,ts_end_to_end_stop;
extern double end_to_end_time[BUFF_LENGTH];
/* transformation time capturing */
extern struct timespec ts_transform_start,ts_transform_stop;
extern double transform_time[BUFF_LENGTH];
/* read frame from camera time capturing */
extern struct timespec ts_read_capture_start,ts_read_capture_stop;
extern double read_capture_time[BUFF_LENGTH];

//funtion prototype (non service)
int process_frame(int *frame_stored);
int save_image(int *frame_stored);


void Sequencer(int id)
{
  int flags=0;

  // received interval timer signal
  if(abortTest)
  {
    // disable interval timer
    itime.it_interval.tv_sec = 0;
    itime.it_interval.tv_nsec = 0;
    itime.it_value.tv_sec = 0;
    itime.it_value.tv_nsec = 0;
    timer_settime(timer_1, flags, &itime, &last_itime);
    printf("Disabling sequencer interval timer with abort=%d and %llu\n", abortTest, seqCnt);

    // shutdown all services
    abortS1=TRUE; abortS2=TRUE; abortS3=TRUE;
    sem_post(&semS1); sem_post(&semS2); sem_post(&semS3);
  }
         
  seqCnt++;
  
  // Release each service at a sub-rate of the generic sequencer rate
  //Service_1 @ 30 Hz = 33.33msec
  if((seqCnt % 30) == 0) sem_post(&semS1); //1sec - > acquisitoin rate 0. (3Hz to 20 Hz for diff logic)

  // Service_2 @ 10 Hz = 100 msec
  //if((seqCnt % 45) == 0) sem_post(&semS2); //1.5 sec - to cause interruption to acq together sometime and sometimes not
  if((seqCnt % 30) == 0) sem_post(&semS2);   //3 sec - read all the messages (for 3 parts)

  // Service_3 @ 1 Hz = 1 second
  if((seqCnt % 30) == 0) sem_post(&semS3); //2sec - to cause interruption to acq together sometime and sometimes not
}

void *Service_1_frame_acquisition(void *threadp)
{
  char buffer[sizeof(void *)+sizeof(int)];
  int nbytes;
  int id = 999;
  
  printf("Code ran Service_1========================\n");

  struct timespec current_time_val;
  double current_realtime;
  unsigned long long S1Cnt=0;

  // Start up processing and resource initialization
  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "S1 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  printf("S1 thread @ sec=%6.9lf\n", current_realtime-start_realtime);

  while(!abortS1) // check for synchronous abort request
  {
    // wait for service request from the sequencer, a signal handler or ISR in kernel
    sem_wait(&semS1);
    
    if(abortS1) break;
      S1Cnt++;
       
    mainloop(); // does only acquisition and memcpy to a buffer
    
    if(framecnt > -1)
    {
      memcpy(tempptr, temp_g_buffer,sizeof(temp_g_buffer));
      memcpy(buffer, &tempptr, sizeof(void *));
      memcpy(&(buffer[sizeof(void *)]), (void *)&id, sizeof(int));
    
      /* send message with priority=30 */
      if((nbytes = mq_send(mymq, buffer, (size_t)(sizeof(void *)+sizeof(int)), 30)) == ERROR)
      {
        perror("Error::TX 1 Message\n");
      }
      else
      {
        //syslog(LOG_CRIT, "Service 1::Messages Sent to WB = %lld\n", S1Cnt);
      }
    }
    
    // on order of up to milliseconds of latency to get time
    clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
    syslog(LOG_CRIT, "S1_SERVICE at 1 Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S1Cnt, (current_realtime-start_realtime));
    
    /* 1 minutes +  15 frames for start up */
    if(S1Cnt >= (WRITEBACK_FRAMES+START_UP_FRAMES) )
    {
      abortS1=TRUE;
      abortTest = TRUE;
      
      // on order of up to milliseconds of latency to get time
      clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
      syslog(LOG_CRIT, "S1_ENDS 1 Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S1Cnt, (current_realtime-start_realtime));
      sem_post(&semS1);
    } 
  }

  // Resource shutdown here
  pthread_exit((void *)0);
}


void *Service_2_frame_process(void *threadp)
{
  printf("Code ran Service_2____________________________\n");
  
  //receive part
  int frame_processed = 0;
  int process_cnt;
  process_cnt = 0;
  
  //send part
  char buffer_send[sizeof(void *)+sizeof(int)];
  int nbytes;
  int id = 999;
  
  struct timespec current_time_val;
  double current_realtime;
  unsigned long long S2Cnt=0;

  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "S2 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  printf("S2 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  
  /* receive logic */    
  mymq = mq_open(SNDRCV_MQ, O_CREAT|O_RDWR, S_IRWXU, &mq_attr);

  while(!abortS2)
  {
    sem_wait(&semS2);

    if(abortS2) break;
      S2Cnt++;
         
    //mq receive and process frame 
    process_cnt = process_frame(&frame_processed);  
      
    //mq send    
    memcpy(buffptr, bigbuffer,sizeof(bigbuffer));
    memcpy(buffer_send, &buffptr, sizeof(void *));
    memcpy(&(buffer_send[sizeof(void *)]), (void *)&id, sizeof(int));
    
    /* send message with priority=30 */
    if((nbytes = mq_send(mymq2, buffer_send, (size_t)(sizeof(void *)+sizeof(int)), 30)) == ERROR)
    {
      perror("Error::TX 2 Message\n");
    }
    else
    {
      //syslog(LOG_CRIT, "Service 2::Messages Sent to 3 = %lld\n", S2Cnt);
    }
    
    // on order of up to milliseconds of latency to get time
    clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
    syslog(LOG_CRIT, "S2_SERVICE at  Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S2Cnt, (current_realtime-start_realtime)); 
      
    #if 0  
    if(S2Cnt >= ((4)*(10) + 4) )
    {
      abortS2=TRUE;
        
      //on order of up to milliseconds of latency to get time
      clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
      syslog(LOG_CRIT, "S2_ENDS  Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S2Cnt, (current_realtime-start_realtime));
          sem_post(&semS2);
    }
    #endif
  }
  pthread_exit((void *)0);
}

int process_frame(int *frame_stored)
{
  char buffer_receive[sizeof(void *)+sizeof(int)];
  void *buffptr_rx; 
  unsigned int prio;
  int nbytes;
  int id;
  int return_val = 0;

  if((nbytes = mq_receive(mymq, buffer_receive, (size_t)((sizeof(void *)+sizeof(int))), &prio)) == ERROR)
  {
    perror("mq_receive 1\n");
    return return_val;
  }
  else
  {
    memcpy(&buffptr_rx, buffer_receive, (sizeof(void *)));
    memcpy((void *)&id, &(buffer_receive[sizeof(void *)]), (sizeof(int)));
    
    
    clock_gettime(CLOCK_MONOTONIC, &ts_transform_start);
   
    process_image(buffptr_rx,614400);
    
    clock_gettime(CLOCK_MONOTONIC, &ts_transform_stop);
    transform_time[framecnt] = dTime(ts_transform_stop, ts_transform_start);
    syslog(LOG_INFO, "transform_time individual is %lf\n", transform_time[framecnt]);   
    
   // aa
    
    return_val = 1;
    *frame_stored = framecnt;
    return return_val; 
  }
  return return_val;
}



//SERVICE 3 START
void *Service_3_frame_storage(void *threadp)
{
  printf("Code ran Service_3++++++++++++++++++++++++++++++\n");
  
  //receive part
  int frame_processed = 0;
  int transform_count;
  transform_count = 0;
  
  //send part
  char buffer_send[sizeof(void *)+sizeof(int)];
  int nbytes;
  int id = 999;
  
  struct timespec current_time_val;
  double current_realtime;
  unsigned long long S3Cnt=0;

  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "S3 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  printf("S3 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  
  /* receive logic */    
  mymq2 = mq_open(SNDRCV_MQ_2, O_CREAT|O_RDWR, S_IRWXU, &mq_attr2);

  while(!abortS3)
  {
    sem_wait(&semS3);

    if(abortS3) break;
      S3Cnt++;
   
    //mq receive and process frame 
    transform_count = process_transform_servicepart(&frame_processed);  

    //mq send    
    memcpy(buffptr_transform, negativebuffer,sizeof(bigbuffer));
    memcpy(buffer_send, &buffptr_transform, sizeof(void *));
    memcpy(&(buffer_send[sizeof(void *)]), (void *)&id, sizeof(int));
    
    /* send message with priority=30 */
    if((nbytes = mq_send(mymq3, buffer_send, (size_t)(sizeof(void *)+sizeof(int)), 30)) == ERROR)
    {
      perror("Error::TX 2 Message\n");
    }
    else
    {
      //syslog(LOG_CRIT, "Service 2::Messages Sent to 3 = %lld\n", S2Cnt);
    }

    
    // on order of up to milliseconds of latency to get time
    clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
    syslog(LOG_CRIT, "S3_SERVICE at Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S3Cnt, (current_realtime-start_realtime));
      
      
      
    #if 0  
    if(S3Cnt >= ((30)*(3) + 6 ))
    {
      //abortS3=TRUE;
      
      // on order of up to milliseconds of latency to get time
      clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
      syslog(LOG_CRIT, "S3_ENDS  Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S3Cnt, (current_realtime-start_realtime));
  
      sem_post(&semS3);
    }
    #endif
  }

  pthread_exit((void *)0);
}

int process_transform_servicepart(int *frame_stored)
{
  char buffer_receive[sizeof(void *)+sizeof(int)];
  void *buffptr_rx; 
  unsigned int prio;
  int nbytes;
  int id;
  int return_val = 0;
  int size = 614400;

  if((nbytes = mq_receive(mymq2, buffer_receive, (size_t)((sizeof(void *)+sizeof(int))), &prio)) == ERROR)
  {
    perror("mq_receive 2\n");
    return return_val;
  }
  else
  {
    memcpy(&buffptr_rx, buffer_receive, (sizeof(void *)));
    memcpy((void *)&id, &(buffer_receive[sizeof(void *)]), (sizeof(int)));
        
    clock_gettime(CLOCK_MONOTONIC, &ts_end_to_end_start);  
    process_transform(buffptr_rx,size);
    
    clock_gettime(CLOCK_MONOTONIC, &ts_end_to_end_stop);
    end_to_end_time[framecnt] = dTime(ts_end_to_end_stop, ts_end_to_end_start);
    syslog(LOG_INFO, "end_to_end_time_negative individual is %lf\n", end_to_end_time[framecnt]);
  
    return_val = 1;
    *frame_stored = framecnt;
    return return_val; 
  }
  return return_val;
}

//SERVICE 3 END



//WRITEBACK START
void *writeback_dump(void *threadp)
{
  printf("Writeback thread ran\n");
  
  int return_val = 0;
  unsigned long long WBCnt=0;
  struct timespec current_time_val;
  double current_realtime;
  int abort_wb = FALSE;
  int frames_stored = 0;
  
  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "WB thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  printf("WBthread @ sec=%6.9lf\n", current_realtime-start_realtime);
  
  /* receive logic */    
  mymq3 = mq_open(SNDRCV_MQ_3 , O_CREAT|O_RDWR, S_IRWXU, &mq_attr3);
  
  while(!abort_wb)
  {
    WBCnt++; 
   
    return_val = save_image(&frames_stored);

    if(return_val == 1)
    {
      clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
      syslog(LOG_CRIT, "WB_THREAD on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), WBCnt, (current_realtime-start_realtime)); 
    }
    
    if(frames_stored == WRITEBACK_FRAMES)
    {
      abort_wb = TRUE;
    }
  }
  
  pthread_exit((void *)0); 
}


int save_image(int *frame_stored)
{
  char buffer[sizeof(void *)+sizeof(int)];
  void *buffptr1; 
  unsigned int prio;
  int nbytes;
  int id;
  int return_val =0;

  if((nbytes = mq_receive(mymq3, buffer, (size_t)((sizeof(void *)+sizeof(int))), &prio)) == ERROR)
  {
    perror("mq_receive 3\n");
    return return_val;
  }
  else
  {
    memcpy(&buffptr1, buffer, (sizeof(void *)));
    memcpy((void *)&id, &(buffer[sizeof(void *)]), (sizeof(int)));
  
    clock_gettime(CLOCK_MONOTONIC, &ts_writeback_start);
    #if COLOR_CONVERT_RGB
    dump_ppm(buffptr1, ((g_framesize*6)/4), framecnt, &frame_time);
    #else
    dump_pgm(buffptr1, (g_framesize/2), framecnt, &frame_time);
    #endif
        
    return_val = 1;
    *frame_stored = framecnt;
    return return_val;   
 
  }
  return return_val;
}

//WRITEBACK END



void message_queue_setup()
{
  /* setup common message q attributes - 1 */
  mq_attr.mq_maxmsg = 100;
  mq_attr.mq_msgsize = sizeof(void *)+sizeof(int);
  
  mq_attr.mq_flags = 0;
  
  mymq = mq_open(SNDRCV_MQ , O_CREAT|O_RDWR, S_IRWXU, &mq_attr);
  
  if(mymq == (mqd_t)ERROR)
  {
    perror("Error to open message queue\n");
  }
  else
  {
    printf("\nMQ 1 created successfully\n");
  }
  
  /* setup common message q attributes  - 2 */
  mq_attr2.mq_maxmsg = 100;
  mq_attr2.mq_msgsize = sizeof(void *)+sizeof(int);
  
  mq_attr2.mq_flags = 0;
  
  mymq2 = mq_open(SNDRCV_MQ_2 , O_CREAT|O_RDWR, S_IRWXU, &mq_attr2);
  
  if(mymq2 == (mqd_t)ERROR)
  {
    perror("Error to open message queue 2\n");
  }
  else
  {
    printf("MQ 2 created successfully\n");
  }
  
  /* setup common message q attributes  - 3 */
  mq_attr3.mq_maxmsg = 100;
  mq_attr3.mq_msgsize = sizeof(void *)+sizeof(int);
  
  mq_attr3.mq_flags = 0;
  
  mymq3 = mq_open(SNDRCV_MQ_3 , O_CREAT|O_RDWR, S_IRWXU, &mq_attr3);
  
  if(mymq3 == (mqd_t)ERROR)
  {
    perror("Error to open message queue 3\n");
  }
  else
  {
    printf("MQ 3 created successfully\n");
  }
}

void message_queue_release()
{
  mq_close(mymq);
  mq_unlink(SNDRCV_MQ);

  mq_close(mymq2);
  mq_unlink(SNDRCV_MQ_2);
  
  mq_close(mymq3);
  mq_unlink(SNDRCV_MQ_3);

}

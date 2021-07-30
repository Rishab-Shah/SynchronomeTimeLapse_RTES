#include "services.h"


extern void *buffptr;
extern unsigned char bigbuffer[(1280*960)];

struct mq_attr mq_attr;
mqd_t mymq;
struct mq_attr mq_attr2;
mqd_t mymq2;

int sid,rid;
//char imagebuff[4096];
extern int framecnt;
extern struct timespec frame_time;
extern int g_framesize;

void message_queue_setup();
void message_queue_release();
extern void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time);

extern char ppm_header[];
extern char ppm_dumpname[];

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

//


void Sequencer(int id)
{
  //struct timespec current_time_val;
  //double current_realtime;
  int /*rc,*/ flags=0;

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

  // Service_1 @ 30 Hz = 33.33msec
  if((seqCnt % 30) == 0) sem_post(&semS1); //1sec

  // Service_2 @ 10 Hz = 100 msec
  if((seqCnt % 45) == 0) sem_post(&semS2); //1.5 sec - to cause interruption to acq together sometime and sometimes not

  // Service_3 @ 1 Hz = 1 second
  if((seqCnt % 60) == 0) sem_post(&semS3); //2sec - to cause interruption to acq together sometime and sometimes not
}

void *Service_1_frame_acquisition(void *threadp)
{
  char buffer[sizeof(void *)+sizeof(int)];

  //int prio;
  int nbytes;
  int id = 999;
  
  printf("Code ran Service_1========================\n");

  struct timespec current_time_val;
  double current_realtime;
  unsigned long long S1Cnt=0;
  //threadParams_t *threadParams = (threadParams_t *)threadp;

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
    
    if(framecnt > -1)
    {
      clock_gettime(CLOCK_MONOTONIC, &ts_end_to_end_start);
    }
    
    mainloop();
    
    if(framecnt > -1)
    {
      clock_gettime(CLOCK_MONOTONIC, &ts_end_to_end_stop);
      end_to_end_time[framecnt] = dTime(ts_end_to_end_stop, ts_end_to_end_start);
      syslog(LOG_INFO, "end_to_end_time_mainloop individual is %lf\n", end_to_end_time[framecnt]);
    }
    
    //memset(imagebuff, 0, sizeof(imagebuff));
    //sprintf(imagebuff, "%d", S1Cnt);
    
    strcpy(buffptr, (char*)bigbuffer);
    
    //printf("Message to send = %s\n", (char *)buffptr);
    //printf("Sending %ld bytes\n", sizeof(buffptr));
    
    memcpy(buffer, &buffptr, sizeof(void *));
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
    
    // on order of up to milliseconds of latency to get time
    clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
    syslog(LOG_CRIT, "S1_SERVICE at 1 Hz on core %d for release %llu @ msec = %6.9lf\n", sched_getcpu(), S1Cnt, (current_realtime-start_realtime)*MSEC_PER_SEC);
    
    /* 3 minutes +  15 frames for start up */
    if(S1Cnt >= (  (((60)*(3)) +  15))  )
    {
      abortS1=TRUE;
      abortTest = TRUE;
      
      // on order of up to milliseconds of latency to get time
      clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
      syslog(LOG_CRIT, "S1_ENDS 1 Hz on core %d for release %llu @ msec = %6.9lf\n", sched_getcpu(), S1Cnt, (current_realtime-start_realtime)*MSEC_PER_SEC);
      sem_post(&semS1);
    } 
  }

  // Resource shutdown here
  pthread_exit((void *)0);
}


void *Service_2_frame_process(void *threadp)
{
  printf("Code ran Service_2____________________________\n");
  
  struct timespec current_time_val;
  double current_realtime;
  unsigned long long S2Cnt=0;
  //int process_cnt;
  //threadParams_t *threadParams = (threadParams_t *)threadp;

  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "S2 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  printf("S2 thread @ sec=%6.9lf\n", current_realtime-start_realtime);

  while(!abortS2)
  {
    sem_wait(&semS2);

    if(abortS2) break;
      S2Cnt++;
  
    // on order of up to milliseconds of latency to get time
    clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
    syslog(LOG_CRIT, "S2_SERVICE at  Hz on core %d for release %llu @ msec = %6.9lf\n", sched_getcpu(), S2Cnt, (current_realtime-start_realtime)*MSEC_PER_SEC);
        
    #if 0  
    if(S2Cnt >= ((4)*(10) + 4) )
    {
      abortS2=TRUE;
        
      //on order of up to milliseconds of latency to get time
      clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
      syslog(LOG_CRIT, "S2_ENDS  Hz on core %d for release %llu @ msec = %6.9lf\n", sched_getcpu(), S2Cnt, (current_realtime-start_realtime)*MSEC_PER_SEC);
          sem_post(&semS2);
    }
    #endif
  }
  pthread_exit((void *)0);
}


void *Service_3_frame_storage(void *threadp)
{
  printf("Code ran Service_3++++++++++++++++++++++++++++++\n");
  
  struct timespec current_time_val;
  double current_realtime;
  unsigned long long S3Cnt=0;
  //int store_cnt;
  //threadParams_t *threadParams = (threadParams_t *)threadp;

  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "S3 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  printf("S3 thread @ sec=%6.9lf\n", current_realtime-start_realtime);

  while(!abortS3)
  {
    sem_wait(&semS3);

    if(abortS3) break;
      S3Cnt++;

    // on order of up to milliseconds of latency to get time
    clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
    syslog(LOG_CRIT, "S3_SERVICE at Hz on core %d for release %llu @ msec = %6.9lf\n", sched_getcpu(), S3Cnt, (current_realtime-start_realtime)*MSEC_PER_SEC);
      
    #if 0  
    if(S3Cnt >= ((30)*(3) + 6 ))
    {
      //abortS3=TRUE;
      
      // on order of up to milliseconds of latency to get time
      clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
      syslog(LOG_CRIT, "S3_ENDS  Hz on core %d for release %llu @ msec = %6.9lf\n", sched_getcpu(), S3Cnt, (current_realtime-start_realtime)*MSEC_PER_SEC);
  
      sem_post(&semS3);
    }
    #endif
  }

  pthread_exit((void *)0);
}


void *writeback_dump(void *threadp)
{
  char buffer[sizeof(void *)+sizeof(int)];
  void *buffptr; 
  unsigned int prio;
  int nbytes;
  //int count = 0;
  int id;
  
  printf("Writeback thread ran\n");
  
  unsigned long long WBCnt=0;
  struct timespec current_time_val;
  double current_realtime;
  
  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "WB thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  printf("WBthread @ sec=%6.9lf\n", current_realtime-start_realtime);
  
  /* receive logic */    
  mymq = mq_open(SNDRCV_MQ , O_CREAT|O_RDWR, S_IRWXU, &mq_attr);
  
  while(!abortS1)
  {
    WBCnt++;
      
    if((nbytes = mq_receive(mymq, buffer, (size_t)((sizeof(void *)+sizeof(int))), &prio)) == ERROR)
    {
      perror("mq_receive 1\n");
    }
    else
    {
      memcpy(&buffptr, buffer, (sizeof(void *)));
      memcpy((void *)&id, &(buffer[sizeof(void *)]), (sizeof(int)));
      
      //printf("receive: ptr msg 0x%p received with priority = %d, length = %d, id = %d\n", buffptr, prio, nbytes, id);
      //syslog(LOG_CRIT, "Service 2::Messages received = %s\n", (char *)buffptr);
      
      if(framecnt > -1)
      {
      
        clock_gettime(CLOCK_MONOTONIC, &ts_writeback_start);
    
        dump_ppm(bigbuffer, ((g_framesize*6)/4), framecnt, &frame_time);
    
        clock_gettime(CLOCK_MONOTONIC, &ts_writeback_stop);
        writeback_time[framecnt] = dTime(ts_writeback_stop, ts_writeback_start);
        
        syslog(LOG_INFO, "writeback_time individual is %lf\n", writeback_time[framecnt]);
        
        syslog(LOG_CRIT, "WB_THREAD on core %d for release %llu @ msec = %6.9lf\n", sched_getcpu(), WBCnt, (current_realtime-start_realtime)*MSEC_PER_SEC); 
      }   
    }
  }
  pthread_exit((void *)0); 
}


void message_queue_setup()
{
  /* setup common message q attributes - 1*/
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
  
  /* setup common message q attributes  - 2*/
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
}

void message_queue_release()
{
  mq_close(mymq);
  mq_close(mymq2);
  
  mq_unlink(SNDRCV_MQ);
  mq_unlink(SNDRCV_MQ_2);

}

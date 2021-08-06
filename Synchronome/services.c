#include "services.h"


void message_queue_setup();
void message_queue_release();

int service_turned_on = 0;
extern void *vpt[20];

//extern void *tempptr;
#if 0
extern unsigned char temp_g_buffer[614400];
#endif
extern unsigned char temp_g_buffer[20][614400];

extern void *buffptr;
extern unsigned char bigbuffer[(1280*960)];

extern void *buffptr_transform;
extern unsigned char negativebuffer[(1280*960)];

int sid,rid;

struct mq_attr mq_attr;
mqd_t mymq;

struct mq_attr mq_attr2;
mqd_t mymq2;

struct mq_attr mq_attr3;
mqd_t mymq3;

extern int start_up_condition;

extern int transform_on_off;
extern int num_of_mallocs;


extern int running_frequency;
int incrementer;


extern void process_transform(const void *p, int size);
extern void process_image(const void *p, int size);


extern void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time);
extern void dump_pgm(const void *p, int size, unsigned int tag, struct timespec *time);

extern int framecnt;
extern struct timespec frame_time;
extern int g_framesize;

// semaphore - start
int abortS0=FALSE, abortS1=FALSE, abortS2=FALSE, abortS3=FALSE,abortS4=FALSE;
sem_t semS0, semS1, semS2, semS3, semS4;
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
extern double read_capture_time[BUFF_LENGTH+1];
/* Negative transforamtion time */
extern struct timespec ts_negative_transformation_time_start,ts_negative_transformation_time_stop;
extern double negative_transformation_time[BUFF_LENGTH];
//funtion prototype (non service)

void process_negative_frame(int *frame_no_to_transfer,struct timespec *st_frame_time_to_transfer);
void process_frame(int *frame_no_to_transfer,struct timespec *st_frame_time_to_transfer);

int save_image(int *frame_stored);
int process_transform_servicepart(int *frame_stored);

void Sequencer(int id)
{
  int flags=0;
  
  sem_post(&semS0);
  
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
    abortS0=TRUE; abortS1=TRUE; abortS2=TRUE; abortS3=TRUE, abortS4=TRUE;
    sem_post(&semS0);sem_post(&semS1); sem_post(&semS2); sem_post(&semS3), sem_post(&semS4);
  }
  
  seqCnt++;
  
}

void *Service_0_Sequencer(void *threadp)
{ 
  printf("Code ran Service_0_#############################\n");

  struct timespec current_time_val;
  double current_realtime;
  unsigned long long S0Cnt=0;

  // Start up processing and resource initialization
  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "S0 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  printf("S0 thread @ sec=%6.9lf\n", current_realtime-start_realtime);

  while((!abortS0)) // check for synchronous abort request
  {
    // wait for service request from the sequencer, a signal handler or ISR in kernel
    sem_wait(&semS0);
    
    if(abortS0) break;
      S0Cnt++;
       
    // Release each service at a sub-rate of the generic sequencer rate
    //Service_1 @ 30 Hz = 33.33msec
    if((S0Cnt % running_frequency) == 0) sem_post(&semS1); //1sec - > acquisitoin rate 0. (3Hz to 20 Hz for diff logic)
    
    // Service_2 @ 10 Hz = 100 msec
    //if((seqCnt % 45) == 0) sem_post(&semS2); //1.5 sec - to cause interruption to acq together sometime and sometimes not
    if((S0Cnt % running_frequency) == 0) sem_post(&semS2);   //3 sec - read all the messages (for 3 parts)
    
    if((S0Cnt % running_frequency) == 0) sem_post(&semS3); 
       
    // Service_3 @ 1 Hz = 1 second
    if((S0Cnt % 15) == 0) sem_post(&semS4); //
       
    // Service_3 @ 1 Hz = 1 second
 
    // on order of up to milliseconds of latency to get time
    //clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
    //syslog(LOG_CRIT, "S0_SERVICE at 1 Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S0Cnt, (current_realtime-start_realtime));
   
  }
  
  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "S0_ENDS 1 Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S0Cnt, (current_realtime-start_realtime));
  printf("service 0 - SEQEUENCER exited\n");
  // Resource shutdown here
  pthread_exit((void *)0);
}

void *Service_1_frame_acquisition(void *threadp)
{
  char buffer[sizeof(void *)+sizeof(int)+sizeof(struct timespec)];
  int nbytes;
  void *tempptr_s1;
  
  printf("Code ran Service_1========================\n");

  struct timespec current_time_val;
  double current_realtime;
  unsigned long long S1Cnt=0;

  // Start up processing and resource initialization
  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "S1 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  printf("S1 thread @ sec=%6.9lf\n", current_realtime-start_realtime);

  while((!abortS1))// check for synchronous abort request
  {
    // wait for service request from the sequencer, a signal handler or ISR in kernel
    sem_wait(&semS1);
    
    if(start_up_condition == 1)
    {
      framecnt = 0;
      start_up_condition = 0;
      S1Cnt = 0;
    } 
    
    if(abortS1) break;
      S1Cnt++;

    mainloop(); // does only acquisition and memcpy to a buffer
    
    if(framecnt > -1)
    { 

      //printf("incrementer = %d\n",incrementer);
      //cb part here only as I have % implemented
      
      
      #if 1
      //printf("size of value is ->%d\n",sizeof(temp_g_buffer[0]));
      tempptr_s1 = (void *)malloc((614400*sizeof(unsigned char)));  
      if(tempptr_s1 == NULL)
      {
        printf("tempptr_s1 malloc failed - %d\n",framecnt);
      }
      memcpy(tempptr_s1, &temp_g_buffer[incrementer],(614400*sizeof(unsigned char)));
      memcpy(buffer, &tempptr_s1, sizeof(void *));
      incrementer++;
      #endif
      
      #if 0
      memcpy(vpt[incrementer], temp_g_buffer,sizeof(temp_g_buffer));
      memcpy(buffer, &vpt[incrementer], sizeof(void *));
      incrementer++;
      #endif

      #if 0
      tempptr_s1 = (void *)malloc(sizeof(temp_g_buffer));
      memcpy(tempptr_s1, temp_g_buffer,sizeof(temp_g_buffer));
      memcpy(buffer, &tempptr_s1, sizeof(void *));
      #endif
      
      memcpy(&(buffer[sizeof(void *)]), &framecnt, sizeof(int));  //global - one time in queue
      memcpy(&(buffer[sizeof(void *) + sizeof(int)]), &frame_time, sizeof(struct timespec)); //global - one time in queue
    
      clock_gettime(CLOCK_MONOTONIC, &ts_read_capture_stop);
      read_capture_time[framecnt] = dTime(ts_read_capture_stop, ts_read_capture_start);
      syslog(LOG_INFO, "read_capture_time individual is %lf\n", read_capture_time[framecnt]);
        
      /* send message with priority=30 */
      if((nbytes = mq_send(mymq, buffer, (size_t)(sizeof(void *)+sizeof(int)+sizeof(struct timespec)), 30)) == ERROR)
      {
        perror("Error::TX 1 Message\n");
      }
      else
      {
        //syslog(LOG_CRIT, "Service 1::Messages Sent to WB = %lld\n", S1Cnt);
      }
      
      framecnt++;
      
    }
     
    // on order of up to milliseconds of latency to get time
    clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
    syslog(LOG_CRIT, "S1_SERVICE at 1 Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S1Cnt, (current_realtime-start_realtime));
    //printf("service 1 - Service_1_frame_acquisition- %d\n",framecnt);
    //printf("frames sent = %d\n",framecnt);
    /* 1 minutes +  15 frames for start up */
    if(framecnt >= (WRITEBACK_FRAMES+1))
    {
      //printf("INSIDE::executed end condition - %d\n",framecnt);
      abortS1=TRUE;
      abortTest=TRUE;
      
      // on order of up to milliseconds of latency to get time
      //clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
      //syslog(LOG_CRIT, "S1_ENDS 1 Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S1Cnt, (current_realtime-start_realtime));
      sem_post(&semS1);
    } 
  }

  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "S1_ENDS 1 Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S1Cnt, (current_realtime-start_realtime));
  printf("service 1 - Service_1_frame_acquisition exited - %d\n",framecnt);
  // Resource shutdown here
  pthread_exit((void *)0);
}

void *Service_2_frame_process(void *threadp)
{
  printf("Code ran Service_2____________________________\n");
  
  //send part
  char buffer_send[sizeof(void *)+sizeof(int)+sizeof(struct timespec)];
  int nbytes;
  void *buffptr_s2;
  
  //receive part
  int l_s2_frame_no_to_send;
  struct timespec l_s2_st_frame_time_to_send;
  
  struct timespec current_time_val;
  double current_realtime;
  unsigned long long S2Cnt=0;

  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "S2 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  printf("S2 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  
  /* receive logic */    
  mymq = mq_open(SNDRCV_MQ, O_CREAT|O_RDWR, S_IRWXU, &mq_attr);

  while((!abortS2))
  {
    sem_wait(&semS2);

    if(abortS2) break;
      S2Cnt++;
         
    //mq receive and process frame 
    process_frame(&l_s2_frame_no_to_send,&l_s2_st_frame_time_to_send);  
      
    //mq send
    #if 1 
    buffptr_s2 = (void *)malloc(sizeof(bigbuffer)); 
    
    if(buffptr_s2 == NULL)
    {
      printf("buffptr_s2 malloc failed - %lld\n",S2Cnt);
    }
    
    memcpy(buffptr_s2, bigbuffer,sizeof(bigbuffer));
    memcpy(buffer_send, &buffptr_s2, sizeof(void *));
    #endif
    
    #if 0
    memcpy(buffptr_transform, negativebuffer,sizeof(negativebuffer));
    memcpy(buffer_send, &buffptr_transform, sizeof(void *));
    #endif
    
    memcpy(&(buffer_send[sizeof(void *)]), &l_s2_frame_no_to_send, sizeof(int));
    memcpy(&(buffer_send[sizeof(void *) + sizeof(int)]), &l_s2_st_frame_time_to_send, sizeof(struct timespec));

    clock_gettime(CLOCK_MONOTONIC, &ts_transform_stop);
    transform_time[framecnt] = dTime(ts_transform_stop, ts_transform_start);
    syslog(LOG_INFO, "transform_time individual is %lf\n", transform_time[l_s2_frame_no_to_send]);
    
    /* send message with priority=30 */
    if((nbytes = mq_send(mymq2, buffer_send, (size_t)(sizeof(void *)+sizeof(int)+sizeof(struct timespec)), 30)) == ERROR)
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
    //printf("service 2 - Service_2_frame_process - %d\n",l_s2_frame_no_to_send);   
    
    #if 0
    if(l_s2_frame_no_to_send >= (WRITEBACK_FRAMES) )
    {
      printf("INSIDE::service 2 - Service_2_frame_process - %d\n",l_s2_frame_no_to_send);
      abortS2=TRUE;
        
      //clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
      //syslog(LOG_CRIT, "S2_ENDS  Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S2Cnt, (current_realtime-start_realtime));
      sem_post(&semS2);
    }
    #endif
    
  }
  
  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "S2_ENDS  Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S2Cnt, (current_realtime-start_realtime));
  printf("service 2 - Service_2_frame_process exited - %d\n",l_s2_frame_no_to_send);
  pthread_exit((void *)0);
}

void process_frame(int *frame_no_to_transfer,struct timespec *st_frame_time_to_transfer)
{
  char buffer_receive[sizeof(void *)+sizeof(int)+sizeof(struct timespec)];
  void *buffptr_rx; 
  
  unsigned int prio;
  int nbytes;
  
  int l_frame_no_extract;
  struct timespec l_st_timespec_extract;
  
  if((nbytes = mq_receive(mymq, buffer_receive, (size_t)((sizeof(void *)+sizeof(int)+sizeof(struct timespec))), &prio)) == ERROR)
  {
    perror("mq_receive 1\n");
  }
  else
  {
  
    clock_gettime(CLOCK_MONOTONIC, &ts_transform_start);
    
    //Extract the data from queue
    memcpy(&buffptr_rx, buffer_receive, (sizeof(void *)));
    memcpy(&l_frame_no_extract, &(buffer_receive[sizeof(void *)]), (sizeof(int)));
    memcpy(&l_st_timespec_extract, &(buffer_receive[sizeof(void *)+sizeof(int)]), (sizeof(struct timespec)));
    
    process_image(buffptr_rx,614400);

    free(buffptr_rx);
    buffptr_rx = NULL;
    
    //For transfer
    *frame_no_to_transfer = l_frame_no_extract;
    st_frame_time_to_transfer->tv_sec  = l_st_timespec_extract.tv_sec;
    st_frame_time_to_transfer->tv_nsec = l_st_timespec_extract.tv_nsec;
  }
}


//modfiy on
void *Service_3_transformation_process(void *threadp)
{
  printf("Code ran Service_3____________________________\n");
  
  //send part
  char buffer_send[sizeof(void *)+sizeof(int)+sizeof(struct timespec)];
  int nbytes;
  void *buffptr_s3;
  
  //receive part
  int l_s3_frame_no_to_send;
  struct timespec l_s3_st_frame_time_to_send;
  
  struct timespec current_time_val;
  double current_realtime;
  unsigned long long S3Cnt=0;

  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "S3 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  printf("S3 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  
  /* receive logic */    
  mymq2 = mq_open(SNDRCV_MQ_2, O_CREAT|O_RDWR, S_IRWXU, &mq_attr2);

  while((!abortS3))
  {
    sem_wait(&semS3);

    if(abortS3) break;
      S3Cnt++;
         
    //mq receive and process frame 
    process_negative_frame(&l_s3_frame_no_to_send,&l_s3_st_frame_time_to_send);  
      
    //mq send
    #if 1 
    buffptr_s3 = (void *)malloc(sizeof(negativebuffer));
    if(buffptr_s3 == NULL)
    {
      printf("buffptr_s3 malloc failed - %lld\n",S3Cnt);
    }
    
    memcpy(buffptr_s3, negativebuffer,sizeof(negativebuffer));
    memcpy(buffer_send, &buffptr_s3, sizeof(void *));
    #endif
    
    memcpy(&(buffer_send[sizeof(void *)]), &l_s3_frame_no_to_send, sizeof(int));
    memcpy(&(buffer_send[sizeof(void *) + sizeof(int)]), &l_s3_st_frame_time_to_send, sizeof(struct timespec));
    
    clock_gettime(CLOCK_MONOTONIC, &ts_negative_transformation_time_stop);
    transform_time[framecnt] = dTime(ts_negative_transformation_time_stop, ts_negative_transformation_time_start);
    syslog(LOG_INFO, "negative_transformation_time individual is %lf\n", negative_transformation_time[framecnt]);
    
    /* send message with priority=30 */
    if((nbytes = mq_send(mymq3, buffer_send, (size_t)(sizeof(void *)+sizeof(int)+sizeof(struct timespec)), 30)) == ERROR)
    {
      perror("Error::TX 2 Message\n");
    }
    else
    {
      //syslog(LOG_CRIT, "Service 2::Messages Sent to 3 = %lld\n", S2Cnt);
    }
    
    // on order of up to milliseconds of latency to get time
    clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
    syslog(LOG_CRIT, "S3_SERVICE at  Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S3Cnt, (current_realtime-start_realtime)); 
    //printf("service 3 Service_3_transformation_process - %d\n",l_s3_frame_no_to_send);
    #if 0
    if(l_s3_frame_no_to_send >= (WRITEBACK_FRAMES))
    {
      printf("INSIDE::service 3 Service_3_transformation_process - %d\n",l_s3_frame_no_to_send);
      abortS3=TRUE;
        
      //on order of up to milliseconds of latency to get time
      clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
      syslog(LOG_CRIT, "S3_ENDS  Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S3Cnt, (current_realtime-start_realtime));
      sem_post(&semS3);
    }
    #endif
  }
  
  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "S2_ENDS 1 Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S3Cnt, (current_realtime-start_realtime));
  
  printf("service 3 Service_3_transformation_process - exited %d\n",l_s3_frame_no_to_send);
  pthread_exit((void *)0);
}

void process_negative_frame(int *frame_no_to_transfer,struct timespec *st_frame_time_to_transfer)
{
  char buffer_receive[sizeof(void *)+sizeof(int)+sizeof(struct timespec)];
  void *buffptr_rx; 
  
  unsigned int prio;
  int nbytes;
  
  int l_frame_no_extract;
  struct timespec l_st_timespec_extract;
  
  if((nbytes = mq_receive(mymq2, buffer_receive, (size_t)((sizeof(void *)+sizeof(int)+sizeof(struct timespec))), &prio)) == ERROR)
  {
    perror("mq_receive 2\n");
  }
  else
  {
    clock_gettime(CLOCK_MONOTONIC, &ts_negative_transformation_time_start);
    
    //Extract the data from queue
    memcpy(&buffptr_rx, buffer_receive, (sizeof(void *)));
    memcpy(&l_frame_no_extract, &(buffer_receive[sizeof(void *)]), (sizeof(int)));
    memcpy(&l_st_timespec_extract, &(buffer_receive[sizeof(void *)+sizeof(int)]), (sizeof(struct timespec)));
    
    process_transform(buffptr_rx,614400);
    
    free(buffptr_rx);
    buffptr_rx = NULL;
    
    //For transfer
    *frame_no_to_transfer = l_frame_no_extract;
    st_frame_time_to_transfer->tv_sec  = l_st_timespec_extract.tv_sec;
    st_frame_time_to_transfer->tv_nsec = l_st_timespec_extract.tv_nsec;
  }
}

//modify off


//SERVICE 3 END

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
  
  while((!abort_wb))
  {
    if(abort_wb) break;
      WBCnt++;
   
    return_val = save_image(&frames_stored);

    if(return_val == 1)
    {
      clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
      syslog(LOG_CRIT, "WB_THREAD on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), WBCnt, (current_realtime-start_realtime)); 
    }
    
    //printf("Frames stored = %d\n",frames_stored);
    
    if(frames_stored >= (WRITEBACK_FRAMES))
    {
      printf("entered - %d\n",frames_stored);
      abort_wb = TRUE;
      abortTest = TRUE;
    }
  }
  
  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "WB_ENDS 1 Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), WBCnt, (current_realtime-start_realtime));
  printf("writeback exited - %d\n",frames_stored);
  pthread_exit((void *)0); 
}


int save_image(int *frame_stored)
{
  char buffer_receive_wb[sizeof(void *)+sizeof(int)+sizeof(struct timespec)];
  void *buffptr_rx_wb;
  int return_val =0;
  
  unsigned int prio;
  int nbytes;
  
  int l_frame_no_extract;
  struct timespec l_st_timespec_extract;
  
  if((nbytes = mq_receive(mymq3, buffer_receive_wb, (size_t)((sizeof(void *)+sizeof(int)+sizeof(struct timespec))), &prio)) == ERROR)
  {
    perror("mq_receive 3\n");
    return return_val;
  }
  else
  {
    //Extract the data from queue
    memcpy(&buffptr_rx_wb, buffer_receive_wb, (sizeof(void *)));
    memcpy(&l_frame_no_extract, &(buffer_receive_wb[sizeof(void *)]), (sizeof(int)));
    memcpy(&l_st_timespec_extract, &(buffer_receive_wb[sizeof(void *)+sizeof(int)]), (sizeof(struct timespec)));
    
    //timestamping - transformation time - either yuyv to rgb or yuyv to negative
    clock_gettime(CLOCK_MONOTONIC, &ts_writeback_start);
    
    #if COLOR_CONVERT_RGB
    dump_ppm(buffptr_rx_wb, ((614400*6)/4), l_frame_no_extract, &l_st_timespec_extract);
    #else
    dump_pgm(buffptr_rx_wb, (614400/2), l_frame_no_extract, &l_st_timespec_extract);
    #endif
    
    free(buffptr_rx_wb);
    buffptr_rx_wb = NULL;
    
    return_val = 1;
    *frame_stored = l_frame_no_extract;
    return return_val;
  }
  return return_val;
}





void message_queue_setup()
{
  /* setup common message q attributes - 1 */
  mq_attr.mq_maxmsg = 100;
  mq_attr.mq_msgsize = sizeof(void *)+sizeof(int)+sizeof(struct timespec);;
  
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
  mq_attr2.mq_msgsize = sizeof(void *)+sizeof(int)+sizeof(struct timespec);;
  
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
  mq_attr3.mq_msgsize = sizeof(void *)+sizeof(int)+sizeof(struct timespec);
  
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

void *Service_4_transformation_on_off(void *threadp)
{
  printf("Code ran Service_3++++++++++++++++++++++++++++++\n");

  struct timespec current_time_val;
  double current_realtime;
  unsigned long long S4Cnt=0;
  char check_info;

  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "S4 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  printf("S4 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
  
  while(!abortS4)
  {
    sem_wait(&semS4);

    //s4
    if(abortS4) break;
      S4Cnt++;
    
    check_info = getchar();
    
    switch(check_info)
    {
      case 't':
      {
        //printf("case t executed\n");
        syslog(LOG_CRIT, "_EXE T CASE\n");
        transform_on_off = !transform_on_off;
        break;
      }
      case 's':
      {
        //printf("case s executed\n");
        syslog(LOG_CRIT, "_EXE S CASE\n");
        start_up_condition = 1;

        break;
      }
      case 'q':
      {
        //printf("case q executed\n");
        syslog(LOG_CRIT, "_EXE Q CASE\n");
        break;
      }
      default :
      {
        printf("detected - %c\n",check_info);
      }
      
      //check_info = '\0';
    }
    
    // on order of up to milliseconds of latency to get time
    clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
    syslog(LOG_CRIT, "S4_SERVICE at Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S4Cnt, (current_realtime-start_realtime));

  }
  
  printf("service 4 Service_4_transformation_on_off exited\n");
  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  syslog(LOG_CRIT, "S4_ENDS 1 Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), S4Cnt, (current_realtime-start_realtime));
  pthread_exit((void *)0);
}


//WRITEBACK END

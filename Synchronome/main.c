// Sam Siewert, December 2020
//
// Sequencer Generic Demonstration
//
// Sequencer - 100 Hz [gives semaphores to all other services]
// Service_1 - 25 Hz, every 4th Sequencer loop reads a V4L2 video frame
// Service_2 -  1 Hz, every 100th Sequencer loop writes out the current video frame
//
// With the above, priorities by RM policy would be:
//
// Sequencer = RT_MAX	@ 100 Hz
// Servcie_1 = RT_MAX-1	@ 25  Hz
// Service_2 = RT_MIN	@ 1   Hz
//

// This is necessary for CPU affinity macros in Linux
#define _GNU_SOURCE


#include "main.h"
//#include "cbfifo.h"
#include "camera_driver.h"
#include "time_spec.h"
#include "services.h"


#define NUM_CPU_CORES          (4)
#define TRUE                   (1)
#define FALSE                  (0)
#define NUM_THREADS            (5)
                              
#define CAMERA_1               (1)
#define WRITEBACK_CORE         (3)


int running_frequency_acquisition;
int fre_20_to_10_hz;
int freq_1_is_to_1;

int number_of_frames_to_store;
int fre_10_to_1_hz;
extern int incrementer;
int start_up_condition;
int transform_on_off;
int num_of_mallocs = 20;

int cpu_core[NUM_THREADS] = {1,1,1,2,2};
// MQ - START

extern void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time);

extern unsigned char bigbuffer[(1280*960)];
void *buffptr;

unsigned char temp_g_buffer[SIZEOF_RING][614400];
//void *tempptr;

void *vpt[20];

extern unsigned char negativebuffer[(1280*960)]; 
void *buffptr_transform;

extern void message_queue_setup();
extern void message_queue_release();
void get_linux_details();
// MQ - END

int running_frequency;
struct timespec start_time_val;

extern int abortTest;
extern int abortS0, abortS1, abortS2, abortS3, abortS4;
extern sem_t semS0, semS1, semS2, semS3, semS4;
extern double start_realtime;
extern timer_t timer_1;
extern struct itimerspec itime;
extern struct itimerspec last_itime;
extern unsigned long long seqCnt;

typedef struct
{
  int threadIdx;
}threadParams_t;

extern void Sequencer(int id);
void set_sequencer_timer_interval();
void print_scheduler(void);
extern void v4l2_frame_acquisition_initialization();
extern void v4l2_frame_acquisition_shutdown();
void init_variables();
//// time stmap varibales

extern double fnow, fstart, fstop, fnow_negative;

extern struct timespec time_now, time_start, time_stop,time_now_negative;

#if 0
/* transformation time capturing */
extern struct timespec ts_transform_start,ts_transform_stop;
extern double transform_time[BUFF_LENGTH];
/* read frame from camera time capturing + 1 for 1801 extra frame */
extern struct timespec ts_read_capture_start,ts_read_capture_stop;
extern double read_capture_time[BUFF_LENGTH+1];

extern double acq_to_tranform_time[BUFF_LENGTH];
#endif

extern void print_analysis();
////

///CAMERA//
extern int fd;
///CAMERA END ////

////////MAIN CHA PART /////////////////////
extern int frame_count;
char       *dev_name;
extern enum io_method io;
#define UNAME_PATH_LENGTH   100
char ppm_uname_string[UNAME_PATH_LENGTH];



int main(int argc, char *argv[])
{
  dev_name = "/dev/video0";
  
  init_variables();

  if(argc > 1)
  {
    /* 10:1 */
    if((strcmp(argv[1],"HZ_1_OFF_180")) == 0)
    {
      running_frequency_acquisition = HZ_10;
      running_frequency = HZ_10;
      fre_10_to_1_hz = 1;
      transform_on_off = 0;
      number_of_frames_to_store = 180;
      
      freq_1_is_to_1 = 0;
      fre_20_to_10_hz = 0;
      printf("10 Hz to 1 Hz and color images as output - 180\n");
      //exit(1);
    }
    /* 10:1 */
    else if((strcmp(argv[1],"HZ_1_ON_180")) == 0)
    {
      running_frequency_acquisition = HZ_10;
      running_frequency = HZ_10;
      fre_10_to_1_hz = 1;
      transform_on_off = 1;
      number_of_frames_to_store = 180;
      
      freq_1_is_to_1 = 0;
      fre_20_to_10_hz = 0;
      printf("10 Hz to 1 Hz and negative images as output - 180\n");
      //exit(1);
    }
    /* 1:1 */
    else if((strcmp(argv[1],"HZ_10_OFF_1800")) == 0)
    {
      running_frequency_acquisition = HZ_10;
      running_frequency = HZ_10;
      fre_10_to_1_hz = 0;
      transform_on_off = 0;
      number_of_frames_to_store = 1800;
      
      freq_1_is_to_1 = 1;
      fre_20_to_10_hz = 0;
      printf("10 Hz everything and color images as output - 1800\n");
      //exit(1);
    }
    /* 1:1 */
    else if((strcmp(argv[1],"HZ_10_ON_1800")) == 0)
    {
      running_frequency_acquisition = HZ_10;
      running_frequency = HZ_10;
      fre_10_to_1_hz = 0;
      transform_on_off = 1;
      number_of_frames_to_store = 1800;
      
      freq_1_is_to_1 = 1;
      fre_20_to_10_hz = 0;
      printf("10 Hz everything and negative images as output - 1800\n");
      //exit(1);
    }
    /* 10:1 */
    else if((strcmp(argv[1],"HZ_1_OFF_1800")) == 0)
    {
      running_frequency_acquisition = HZ_10;
      running_frequency = HZ_10;
      fre_10_to_1_hz = 1;
      transform_on_off = 0;
      number_of_frames_to_store = 1800;
      
      freq_1_is_to_1 = 0;
      fre_20_to_10_hz = 0;
      printf("10 Hz to 1 Hz and negative images as output - 1800 \n");
      //exit(1);
    }
    /* 10:1 */
    else if((strcmp(argv[1],"HZ_1_ON_1800")) == 0)
    {
      running_frequency_acquisition = HZ_10;
      running_frequency = HZ_10;
      fre_10_to_1_hz = 1;
      transform_on_off = 1;
      number_of_frames_to_store = 1800;
      
      freq_1_is_to_1 = 0;
      fre_20_to_10_hz = 0;
      printf("10 Hz to 1 Hz and negative images as output - 1800\n");
      //exit(1);
    }
    else if((strcmp(argv[1],"EXP")) == 0)
    {
      running_frequency_acquisition = HZ_20;
      running_frequency = HZ_20;
      
      fre_10_to_1_hz = 0;
      transform_on_off = 1;
      number_of_frames_to_store = 1800;
      freq_1_is_to_1 = 1;
      fre_20_to_10_hz = 0;
      printf("20 Hz to 10 Hz and negative images as output - 1800\n");
      //exit(1);
    }
    else if((strcmp(argv[1],"unlink")) == 0)
    {
      mq_unlink(SNDRCV_MQ);
      mq_unlink(SNDRCV_MQ_2);
      mq_unlink(SNDRCV_MQ_3);
      exit(1);
    }
    else if((strcmp(argv[1],"help")) == 0)
    {
      printf("HELP\n");
      printf("DEMO::10 Hz to 1 Hz and color images as output        -- sudo ./run HZ_1_OFF_180\n");
      printf("DEMO::10 Hz to 1 Hz and negative images as output     -- sudo ./run HZ_1_ON_180\n");
      printf("10 Hz everything and color images as output     -- sudo ./run HZ_10_OFF_1800\n");
      printf("10 Hz everything and negative images as output  -- sudo ./run HZ_10_ON_1800\n");
      printf("10 Hz to 1 Hz and color images as output        -- sudo ./run HZ_1_OFF_1800\n");
      printf("10 Hz to 1 Hz and negative images as output     -- sudo ./run HZ_1_ON_1800\n");
      exit(1);
    }
    else
    {
      printf("Please enter correct command\n");
      exit(1);
    }
  }
  else
  { 
    printf("Please enter an option.\n For more information enter sudo ./run help\n");
    exit(1);
  }

  message_queue_setup();
  
  struct timespec current_time_val, current_time_res;
  double current_realtime, current_realtime_res;

  int i,rc, scope;

  cpu_set_t threadcpu;
  cpu_set_t allcpuset;
      
  pthread_t threads[NUM_THREADS];
  threadParams_t threadParams[NUM_THREADS];
  pthread_attr_t rt_sched_attr[NUM_THREADS];

  int rt_max_prio, rt_min_prio, cpuidx;
  
  struct sched_param rt_param[NUM_THREADS];
  struct sched_param main_param;

  pthread_attr_t main_attr;
  pid_t mainpid;

  // initialization of V4L2
  #if CAMERA_1
  v4l2_frame_acquisition_initialization();
  #endif

  printf("Starting High Rate Sequencer Demo\n");
  clock_gettime(MY_CLOCK_TYPE, &start_time_val); start_realtime=realtime(&start_time_val);
  clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
  clock_getres(MY_CLOCK_TYPE, &current_time_res); current_realtime_res=realtime(&current_time_res);
  printf("START High Rate Sequencer @ sec=%6.9lf with resolution %6.9lf\n", (current_realtime - start_realtime), current_realtime_res);
  syslog(LOG_CRIT, "START High Rate Sequencer @ sec=%6.9lf with resolution %6.9lf\n", (current_realtime - start_realtime), current_realtime_res);
  printf("System has %d processors configured and %d available.\n", get_nprocs_conf(), get_nprocs());

  CPU_ZERO(&allcpuset);

  for(i=0; i < NUM_CPU_CORES; i++)
     CPU_SET(i, &allcpuset);

  printf("Using CPUS=%d from total available.\n", CPU_COUNT(&allcpuset));

  // initialize the sequencer semaphores
  if (sem_init (&semS0, 0, 0)) { printf ("Failed to initialize S0 semaphore\n"); exit (-1); }
  if (sem_init (&semS1, 0, 0)) { printf ("Failed to initialize S1 semaphore\n"); exit (-1); }
  if (sem_init (&semS2, 0, 0)) { printf ("Failed to initialize S2 semaphore\n"); exit (-1); }
  if (sem_init (&semS3, 0, 0)) { printf ("Failed to initialize S3 semaphore\n"); exit (-1); }
  if (sem_init (&semS4, 0, 0)) { printf ("Failed to initialize S4 semaphore\n"); exit (-1); }

  mainpid = getpid();

  rt_max_prio = sched_get_priority_max(SCHED_FIFO);
  rt_min_prio = sched_get_priority_min(SCHED_FIFO);

  rc=sched_getparam(mainpid, &main_param);
  main_param.sched_priority=rt_max_prio;
  rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
  if(rc < 0) perror("main_param");
  print_scheduler();

  pthread_attr_getscope(&main_attr, &scope);

  if(scope == PTHREAD_SCOPE_SYSTEM)
    printf("PTHREAD SCOPE SYSTEM\n");
  else if (scope == PTHREAD_SCOPE_PROCESS)
    printf("PTHREAD SCOPE PROCESS\n");
  else
    printf("PTHREAD SCOPE UNKNOWN\n");

  printf("rt_max_prio=%d\n", rt_max_prio);
  printf("rt_min_prio=%d\n", rt_min_prio);

  for(i = 0; i < NUM_THREADS; i++)
  {
    // run ALL threads on core RT_CORE
    CPU_ZERO(&threadcpu);
    cpuidx = cpu_core[i];
    CPU_SET(cpuidx, &threadcpu);

    rc=pthread_attr_init(&rt_sched_attr[i]);
    rc=pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
    rc=pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);
    rc=pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

    threadParams[i].threadIdx=i;
    print_scheduler();
  }

  printf("Service threads will run on %d CPU cores\n",CPU_COUNT(&threadcpu));
  
  //writeback
  cpu_set_t cpuset_wb;
  int cpuidx_wb;
  //writeback
  pthread_t writeback_thread;
  threadParams_t writeback_threadparam;
  pthread_attr_t writeback_sched_attr;
  //writeback CPU affinity
  pthread_attr_init(&writeback_sched_attr);
  CPU_ZERO(&cpuset_wb);
  cpuidx_wb=(WRITEBACK_CORE);
  CPU_SET(cpuidx_wb, &cpuset_wb);
  pthread_attr_setaffinity_np(&writeback_sched_attr, sizeof(cpu_set_t), &cpuset_wb);
  
  // Create Service threads which will block awaiting release for:
  
  //Service_0 - sequencer
  rt_param[0].sched_priority=rt_max_prio-1;
  pthread_attr_setschedparam(&rt_sched_attr[0], &rt_param[0]);
  rc=pthread_create(&threads[0],&rt_sched_attr[0], Service_0_Sequencer,(void *)&(threadParams[0]));
  if(rc < 0)
      perror("pthread_create for service 0 - Sequencer\n");
  else
      printf("pthread_create successful for service 0\n");

  // Servcie_1 = Acqusition
  rt_param[1].sched_priority=rt_max_prio-2;
  pthread_attr_setschedparam(&rt_sched_attr[1], &rt_param[1]);
  rc=pthread_create(&threads[1],&rt_sched_attr[1], Service_1_frame_acquisition,(void *)&(threadParams[1]));
  if(rc < 0)
      perror("pthread_create for service 1 - V4L2 video frame acquisition\n");
  else
      printf("pthread_create successful for service 1\n");
      
  // Service_2 = YUYV to RGB
  rt_param[2].sched_priority=rt_max_prio-3;
  pthread_attr_setschedparam(&rt_sched_attr[2], &rt_param[2]);
  rc=pthread_create(&threads[2], &rt_sched_attr[2], Service_2_frame_process, (void *)&(threadParams[2]));
  if(rc < 0)
      perror("pthread_create for service 2 - flash frame storage\n");
  else
      printf("pthread_create successful for service 2\n");
  pthread_detach(threads[2]); //color

  rt_param[3].sched_priority=rt_max_prio-4;
  pthread_attr_setschedparam(&rt_sched_attr[3], &rt_param[3]);
  rc=pthread_create(&threads[3], &rt_sched_attr[3], Service_3_transformation_process, (void *)&(threadParams[3]));
  if(rc < 0)
      perror("pthread_create for service 3 - rgb to negative\n");
  else
      printf("pthread_create successful for service 3\n");
  pthread_detach(threads[3]); //transformation
      
  // Service_3 = transform on/off
  rt_param[4].sched_priority=rt_max_prio-5;
  pthread_attr_setschedparam(&rt_sched_attr[4], &rt_param[4]);
  rc=pthread_create(&threads[4], &rt_sched_attr[4], Service_4_transformation_on_off, (void *)&(threadParams[4]));
  if(rc < 0)
      perror("pthread_create for service 4 - transform on off\n");
  else
      printf("pthread_create successful for service 4\n");
  pthread_detach(threads[4]); //getchar()
  
  writeback_threadparam.threadIdx = 8;
  pthread_create(&writeback_thread, &writeback_sched_attr, writeback_dump, (void *)&writeback_threadparam);
      
  set_sequencer_timer_interval();
  
  for(i=0;i<NUM_THREADS;i++)
  {
    if((rc=pthread_join(threads[i], NULL) < 0))
    {
      perror("main pthread_join");
    }
    else
    {
      printf("joined thread %d\n", i);
    }
  }
   
  // writeback thread
  if((rc=pthread_join(writeback_thread, NULL) < 0))
  {
    perror("main pthread_join - 1");
  }
  else
  {
    printf("WB thread joined\n");
  }
  
  
#if CAMERA_1
  //shutdown of frame acquisition service
  v4l2_frame_acquisition_shutdown();
  //print_analysis();
  fprintf(stderr, "\n");
#endif

  message_queue_release();
  
  #if 0
  for(int i = 0; i < num_of_mallocs; i++)
  {
     free(vpt[i]);
     vpt[i] = NULL;
  }
  #endif
  
  free(buffptr);
  buffptr = NULL;
  
  free(buffptr_transform);
  buffptr_transform = NULL;
  
  printf("heap space memory freed\n");
  printf("CAMERA_TEST COMPLETE\n");
  
  return 0;
}

void errno_exit(const char *s)
{
  fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
  exit(EXIT_FAILURE);
}


void print_scheduler(void)
{
  int schedType;
  
  schedType = sched_getscheduler(getpid());
  
  switch(schedType)
  {
    case SCHED_FIFO:
      printf("Pthread Policy is SCHED_FIFO\n");
      break;
    case SCHED_OTHER:
      printf("Pthread Policy is SCHED_OTHER\n");
      exit(-1);
      break;
    case SCHED_RR:
      printf("Pthread Policy is SCHED_RR\n");
      exit(-1);
      break;
    default:
      printf("Pthread Policy is UNKNOWN\n");
      exit(-1);
  }
}

void set_sequencer_timer_interval()
{
  int flags=0;
  
  /* set up to signal SIGALRM if timer expires */
  timer_create(CLOCK_REALTIME, NULL, &timer_1);
  signal(SIGALRM, (void(*)()) Sequencer);
  
  /* arm the interval timer */
  itime.it_interval.tv_sec = 0;
  itime.it_interval.tv_nsec = COUNT_TO_LOAD;
  itime.it_value.tv_sec = 0;
  itime.it_value.tv_nsec = COUNT_TO_LOAD;
  
  timer_settime(timer_1, flags, &itime, &last_itime);
}


void init_variables()
{
  abortS0=FALSE; abortS1=FALSE; abortS2=FALSE; abortS3=FALSE; abortS4=FALSE;
  seqCnt = 0;
  
  running_frequency = HZ_1;//1 hz
  incrementer = 0;
  num_of_mallocs = 20;
  transform_on_off = 1;
  start_up_condition = 0;
  fre_10_to_1_hz = 1;
  
  #if 0
  for(int i = 0; i < num_of_mallocs; i++)
  {
    vpt[i] = (void *)malloc(sizeof(temp_g_buffer));
    
    if(NULL == vpt[i])
    {
      printf("Malloc failed at %d\n",i);
    }
  }
  #endif
  
  //tempptr = (void *)malloc(sizeof(temp_g_buffer));
  buffptr = (void *)malloc(sizeof(bigbuffer));
  buffptr_transform = (void *)malloc(sizeof(negativebuffer));
  
  get_linux_details(); 
}


void get_linux_details()
{
  FILE *fp;
  char path[UNAME_PATH_LENGTH];
  
  strcpy(path,"");
  strcpy(ppm_uname_string,"");
  
  fp = popen("uname -a","r");
  if (fp == NULL)
      /* Handle error */;
  
  while(fgets(path, UNAME_PATH_LENGTH, fp) != NULL)
      printf("%s", path);
  
  strcpy(ppm_uname_string,"#");
  strncat(ppm_uname_string,path,strlen(path)); 
  pclose(fp);

}

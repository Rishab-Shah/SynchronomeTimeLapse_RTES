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
#include "cbfifo.h"
#include "camera_driver.h"
#include "time_spec.h"
#include "services.h"


#define NUM_CPU_CORES          (4)
#define TRUE                   (1)
#define FALSE                  (0)
#define NUM_THREADS            (3)
                              
#define CAMERA_1               (1)
#define WRITEBACK_CORE         (3)

int cpu_core[NUM_THREADS] = {1,2,2};
// MQ - START

extern char imagebuff[4096];
extern void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time);

extern unsigned char bigbuffer[(1280*960)];
void *buffptr;

unsigned char temp_g_buffer[614400];
void *tempptr;

extern unsigned char negativebuffer[(1280*960)];
void *buffptr_transform;

extern void message_queue_setup();
extern void message_queue_release();
void get_linux_details();
// MQ - END

int running_frequency = 30;
struct timespec start_time_val;

extern int abortTest;
extern int abortS1, abortS2, abortS3;
extern sem_t semS1, semS2, semS3;
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
static int out_buf;
static int force_format = 1;
//static enum io_method   io = IO_METHOD_USERPTR;
//static enum io_method   io = IO_METHOD_READ;
extern enum io_method io;
#define UNAME_PATH_LENGTH   100
char ppm_uname_string[UNAME_PATH_LENGTH];



int main(int argc, char *argv[])
{
  dev_name = "/dev/video0";
  int compare = 0;
  
  //update later
  if(argc > 2)
  {
    sscanf(argv[2], "%d", &compare);
    if(compare == 10)
    {
      running_frequency = 3;
      printf("Running Frequency = 10 Hz\n");
    }
    else if(compare == 1)
    {
      running_frequency = 30;
      printf("Running Frequency = 1 Hz\n");
    }
    else
    {
      printf("Running Frequency = 1 Hz\n");
    }
  }
  else if(argc < 2)
  {
    printf("else if -> Running Frequency = 1 Hz\n");
    /* Do nothing */
  }
  else
  {
    printf("else\n");
  }
  
  
  init_variables();
  
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

  //required to get camera initialized and ready
  //seq_frame_read();

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
  if (sem_init (&semS1, 0, 0)) { printf ("Failed to initialize S1 semaphore\n"); exit (-1); }
  if (sem_init (&semS2, 0, 0)) { printf ("Failed to initialize S2 semaphore\n"); exit (-1); }
  if (sem_init (&semS3, 0, 0)) { printf ("Failed to initialize S2 semaphore\n"); exit (-1); }

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
    //cpuidx=(RT_CORE);
    cpuidx = cpu_core[i];
    CPU_SET(cpuidx, &threadcpu);

    rc=pthread_attr_init(&rt_sched_attr[i]);
    rc=pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
    rc=pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);
    rc=pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

    rt_param[i].sched_priority=rt_max_prio-i;
    pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

    threadParams[i].threadIdx=i;
    print_scheduler();
  }

  printf("Service threads will run on %d CPU cores\n", CPU_COUNT(&threadcpu));
  
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

  // Servcie_1 = RT_MAX-1	@ 20 Hz
  rt_param[0].sched_priority=rt_max_prio-1;
  pthread_attr_setschedparam(&rt_sched_attr[0], &rt_param[0]);
  rc=pthread_create(&threads[0],&rt_sched_attr[0], Service_1_frame_acquisition,(void *)&(threadParams[0]));
  if(rc < 0)
      perror("pthread_create for service 1 - V4L2 video frame acquisition");
  else
      printf("pthread_create successful for service 1\n");
      
  // Service_2 = RT_MAX-2	@ 10 Hz
  rt_param[1].sched_priority=rt_max_prio-2;
  pthread_attr_setschedparam(&rt_sched_attr[1], &rt_param[1]);
  rc=pthread_create(&threads[1], &rt_sched_attr[1], Service_2_frame_process, (void *)&(threadParams[1]));
  if(rc < 0)
      perror("pthread_create for service 2 - flash frame storage");
  else
      printf("pthread_create successful for service 2\n");

  // Service_3 = RT_MAX-3	@ 1 Hz
  rt_param[2].sched_priority=rt_max_prio-3;
  pthread_attr_setschedparam(&rt_sched_attr[2], &rt_param[2]);
  rc=pthread_create(&threads[2], &rt_sched_attr[2], Service_3_frame_storage, (void *)&(threadParams[2]));
  if(rc < 0)
      perror("pthread_create for service 3 - flash frame storage");
  else
      printf("pthread_create successful for service 3\n");

  writeback_threadparam.threadIdx = 5;
  pthread_create(&writeback_thread, &writeback_sched_attr, writeback_dump, (void *)&writeback_threadparam);
  
  //sleep(1);
  
  char c;
  printf("\n\nShotgun mode\n\n");
  c = getchar();
  printf("executed\n");
  putchar(c);
  
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
  print_analysis();
  fprintf(stderr, "\n");
#endif

  message_queue_release();
  
  free(buffptr);
  free(tempptr);
  free(buffptr_transform);
  
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
  itime.it_interval.tv_nsec = 33333333;
  itime.it_value.tv_sec = 0;
  itime.it_value.tv_nsec = 33333333;
  
  timer_settime(timer_1, flags, &itime, &last_itime);
}


void init_variables()
{
  abortS1=FALSE; abortS2=FALSE; abortS3=FALSE;
  seqCnt = 0;
  
  buffptr = (void *)malloc(sizeof(bigbuffer));
  tempptr = (void *)malloc(sizeof(temp_g_buffer));
  buffptr_transform = (void *)malloc(sizeof(negativebuffer));
  
  get_linux_details(); 
}


void get_linux_details()
{
  FILE *fp;
  int status;
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
  status = pclose(fp);
  
  if(status == -1)
  {
    /* Error reported by pclose() */
  }
  else
  {
    /* Use macros described under wait() to inspect `status' in order
    to determine success/failure of command executed by popen() */
  }
  
}

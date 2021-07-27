#include "services.h"

int abortS1=FALSE, abortS2=FALSE, abortS3=FALSE;
sem_t semS1, semS2, semS3;
 int abortTest=FALSE;
 timer_t timer_1;
 struct itimerspec last_itime;
struct itimerspec itime = {{1,0}, {1,0}};
double start_realtime;
unsigned long long seqCnt=0;
void Sequencer(int id);

extern void mainloop(void);

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

    // Servcie_1 @ 30 Hz = 33.33msec
    /*if((seqCnt % 3.33) == 0)*/ 
      sem_post(&semS1);

    // Service_2 @ 10 Hz = 100 msec
    if((seqCnt % 3) == 0) sem_post(&semS2);

    // Service_3 @ 1 Hz = 1 second
    if((seqCnt % 30) == 0) sem_post(&semS3);
}

void *Service_1_frame_acquisition(void *threadp)
{
    printf("Code ran \n");
    mainloop();
    #if 0
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
         //printf("Code ran 1\n");
        //sem_wait(&semS1);

 //printf("Code ran 2 \n");
        if(abortS1) break;
            S1Cnt++;

        // on order of up to milliseconds of latency to get time
        clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
        syslog(LOG_CRIT, "S1_SERVICE at 30 Hz on core %d for release %llu @ msec=%6.9lf\n", sched_getcpu(), S1Cnt, (current_realtime-start_realtime)*MSEC_PER_SEC);

        if(S1Cnt >= (30)*(60))
        {
            abortS1=TRUE;
            
            
                    // on order of up to milliseconds of latency to get time
        clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
        syslog(LOG_CRIT, "S1_ENDS 30 Hz on core %d for release %llu @ msec=%6.9lf\n", sched_getcpu(), S1Cnt, (current_realtime-start_realtime)*MSEC_PER_SEC);
            sem_post(&semS1);
            

        } 
    }

    // Resource shutdown here
    //
    #endif
    pthread_exit((void *)0);
}


void *Service_2_frame_process(void *threadp)
{

    
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
        syslog(LOG_CRIT, "S2_SERVICE at 10 Hz on core %d for release %llu @ msec=%6.9lf\n", sched_getcpu(), S2Cnt, (current_realtime-start_realtime)*MSEC_PER_SEC);
            
        if(S2Cnt >= (60)*(10))
        {
            abortS2=TRUE;
            
        // on order of up to milliseconds of latency to get time
        clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
        syslog(LOG_CRIT, "S2_ENDS 10 Hz on core %d for release %llu @ msec=%6.9lf\n", sched_getcpu(), S2Cnt, (current_realtime-start_realtime)*MSEC_PER_SEC);
            sem_post(&semS2);
        }
            
          
    }

    pthread_exit((void *)0);
}


void *Service_3_frame_storage(void *threadp)
{
    
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
        syslog(LOG_CRIT, "S3_SERVICE at 1 Hz on core %d for release %llu @ msec=%6.9lf\n", sched_getcpu(), S3Cnt, (current_realtime-start_realtime)*MSEC_PER_SEC);
            
        if(S3Cnt >= 60)
        {
            abortS3=TRUE;
            
                    // on order of up to milliseconds of latency to get time
            clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
            syslog(LOG_CRIT, "S3_ENDS 1 Hz on core %d for release %llu @ msec=%6.9lf\n", sched_getcpu(), S3Cnt, (current_realtime-start_realtime)*MSEC_PER_SEC);
        
            sem_post(&semS3);
        }
    }

    pthread_exit((void *)0);
}


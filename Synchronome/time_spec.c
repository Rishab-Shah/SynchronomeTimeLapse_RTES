#include "time_spec.h"


double getTimeMsec(void)
{
  struct timespec event_ts = {0, 0};

  clock_gettime(MY_CLOCK_TYPE, &event_ts);
  return ((event_ts.tv_sec)*MSEC_PER_SEC) + ((event_ts.tv_nsec)/NANOSEC_PER_MSEC);
}


double realtime(struct timespec *tsptr)
{
    /* Getting everything in seconds */
    return ((double)(tsptr->tv_sec) + (((double)tsptr->tv_nsec)/NANOSEC_PER_SEC));
}

double dTime(struct timespec now, struct timespec start)
{
  double nowReal=0.0, startReal=0.0;

  nowReal = (double)now.tv_sec + ((double)now.tv_nsec / NANOSEC_PER_SEC);
  startReal = (double)start.tv_sec + ((double)start.tv_nsec / NANOSEC_PER_SEC);

  return (nowReal-startReal);
}
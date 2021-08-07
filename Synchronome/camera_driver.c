#include "camera_driver.h"

#include <stdbool.h>
#include <assert.h>
#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include "time_spec.h"

//#define FRAMES_PER_SEC        (1) 
//#define FRAMES_PER_SEC        (10) 
//#define FRAMES_PER_SEC        (20) 
//#define FRAMES_PER_SEC        (25) 
#define FRAMES_PER_SEC          (30) 


#define HRES                    (640)
#define VRES                    (480)
#define HRES_STR                "640"
#define VRES_STR                "480"


#define CLEAR(x)                memset(&(x), 0, sizeof(x))


#define DUMP_FRAMES
#define UNPROCESSED_FRAMES      (1)
#define DRIVER_MMAP_BUFFERS     (6)

extern int incrementer;
extern int start_up_condition;
extern int transform_on_off;
extern unsigned char temp_g_buffer[20][614400];

struct timespec frame_time;

struct buffer 
{
  void   *start;
  size_t  length;
};

void process_transform(const void *p, int size);

extern char ppm_uname_string[100];


double fnow=0.0, fstart=0.0, fstop=0.0, fnow_negative = 0.0;

struct timespec time_now, time_start, time_stop,time_now_negative;

struct buffer    *buffers;
unsigned int     n_buffers;
int frame_count = (FRAMES_TO_ACQUIRE);
extern char *dev_name;
int fd = -1;
enum io_method   io = IO_METHOD_MMAP;
struct v4l2_format fmt;
const int force_format=1;
// always ignore first 8 frames 
int framecnt = -START_UP_FRAMES;
int g_framesize;

unsigned char bigbuffer[(1280*960)];
unsigned char negativebuffer[(1280*960)];

/* transformation time capturing */
struct timespec ts_transform_start,ts_transform_stop;
double transform_time[BUFF_LENGTH];
/* writeback time capturing */
struct timespec ts_writeback_start,ts_writeback_stop;
double writeback_time[BUFF_LENGTH];
/* read frame from camera time capturing */
struct timespec ts_read_capture_start,ts_read_capture_stop;
double read_capture_time[BUFF_LENGTH+1];
/* end to end time */
struct timespec ts_negative_transformation_time_start,ts_negative_transformation_time_stop;
double negative_transformation_time[BUFF_LENGTH];

double acq_to_tranform_time[BUFF_LENGTH];


void v4l2_frame_acquisition_initialization();

void mainloop(void);
void process_image(const void *p, int size);
int read_frame(void);
void camera_opening();
void camera_closing();
void start_capturing(void);
void stop_capturing(void);
void init_userp(unsigned int buffer_size);
void init_mmap(void);
void init_read(unsigned int buffer_size);
void open_device(void);
int xioctl(int fh, int request, void *arg);
void init_device(void);
void uninit_device(void);
void close_device(void);
void print_analysis();



//






//

void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time);
//static void dump_ppm_modified(const void *p, int size, unsigned int tag, struct timespec *time);
void dump_pgm(const void *p, int size, unsigned int tag, struct timespec *time);
void yuv2rgb_float(float y, float u, float v, unsigned char *r, unsigned char *g, unsigned char *b);
void yuv2rgb(int y, int u, int v, unsigned char *r, unsigned char *g, unsigned char *b);

void negative_transformation(unsigned char colored_r, unsigned char colored_g, unsigned char colored_b,unsigned char *negative_r, unsigned char *negative_g, unsigned char *negative_b);
#if 0
void negative_transformation(unsigned char *colored_r, unsigned char *colored_g, unsigned char *colored_b,unsigned char *negative_r, unsigned char *negative_g, unsigned char *negative_b);
#endif
void mainloop(void)
{  
  if(framecnt == 0) 
  {
    clock_gettime(CLOCK_MONOTONIC, &time_start);
    fstart = (double)time_start.tv_sec + (double)time_start.tv_nsec / NANOSEC_PER_SEC;
  }
  
  fd_set fds;
  struct timeval tv;
  int rc;
  
  FD_ZERO(&fds);
  FD_SET(fd, &fds);
  
  /* Timeout */
  tv.tv_sec = 2;
  tv.tv_usec = 0;
  
  rc = select(fd + 1, &fds, NULL, NULL, &tv);
  
  if(-1 == rc)
  {
    if (EINTR == errno)
      errno_exit("select");
    else
      printf("Does this execute\n");
  }
  else if(0 == rc)
  {
    fprintf(stderr, "select timeout\n");
    exit(EXIT_FAILURE);
  }
  else
  {
    /* Do Nothing */
  }
  
  //start
  if(framecnt > -1) 
  {
    clock_gettime(CLOCK_MONOTONIC, &ts_read_capture_start);
  }
  
            
  if(read_frame())
  {
    //Some storing operation required.
    if(framecnt > -1)
    {	
      clock_gettime(CLOCK_MONOTONIC, &time_now);
      fnow = (double)time_now.tv_sec + (double)time_now.tv_nsec / NANOSEC_PER_SEC;
      syslog(LOG_INFO, "read_frame_read_at %lf @ %lf FPS\n", (fnow-fstart), (double)(framecnt+1) / (fnow-fstart));
    }
    else 
    {
      syslog(LOG_INFO, "at %lf\n", fnow);
    }
  }
  else
  {
    printf("Read did not happen\n");
  }
  
  //end
}


int read_frame()
{
  struct v4l2_buffer buf;
  unsigned int i;

  switch(io)
  {
    case IO_METHOD_MMAP:
    {
      CLEAR(buf);
      
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      
      if(-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
      {
        switch (errno)
        {
          case EAGAIN:
            return 0;
          case EIO:
            /* Could ignore EIO, but drivers should only set for serious errors, although some set for
            non-fatal errors too. */
            return 0;
          default:
            printf("mmap failure\n");
            errno_exit("VIDIOC_DQBUF");
        }
      }
      
      assert(buf.index < n_buffers);
      
      //process_image(buffers[0].start, buffers[0].length);

      //printf("frame count is %d\n",framecnt);
      
      if(framecnt > -1) 
      {
        if(incrementer % (20) == 0)
        {
          incrementer = 0;
        }
      
        clock_gettime(CLOCK_REALTIME, &frame_time);
        
        memcpy((void *)&temp_g_buffer[incrementer][0],buffers[buf.index].start,buf.bytesused);
        
      }
      
      if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
        errno_exit("VIDIOC_QBUF");
        
      break;
    }
    
    case IO_METHOD_READ:
    {
      if (-1 == read(fd, buffers[0].start, buffers[0].length))
      {
        switch (errno)
        {
          case EAGAIN:
            return 0;
          case EIO:
            /* Could ignore EIO, see spec. */
            /* fall through */
          default:
            errno_exit("read");
        }
      }
      
      process_image(buffers[0].start, buffers[0].length);
      break;
    }

    case IO_METHOD_USERPTR:
    {
      CLEAR(buf);
      
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_USERPTR;
      
      if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
      {
        switch (errno)
        {
          case EAGAIN:
            return 0;
          case EIO:
            /* Could ignore EIO, see spec. */
            /* fall through */
          default:
            errno_exit("VIDIOC_DQBUF");
        }
      }
      
      for (i = 0; i < n_buffers; ++i)
      if (buf.m.userptr == (unsigned long)buffers[i].start && buf.length == buffers[i].length)
        break;
      
      assert(i < n_buffers);
      
      process_image((void *)buf.m.userptr, buf.bytesused);
      
      if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
      errno_exit("VIDIOC_QBUF");
      break;
    }
  }
  return 1;
}


#if 1
//original
#define SAT                   (255)
void negative_transformation(unsigned char colored_r, unsigned char colored_g, unsigned char colored_b,unsigned char *negative_r, unsigned char *negative_g, unsigned char *negative_b)
{
  *negative_r = SAT - colored_r;
  *negative_g = SAT - colored_g;
  *negative_b = SAT - colored_b;
}

#endif


#if 0
#define SAT                   (255)
void negative_transformation(unsigned char *colored_r, unsigned char *colored_g, unsigned char *colored_b,unsigned char *negative_r, unsigned char *negative_g, unsigned char *negative_b)
{
  *negative_r = SAT - *colored_r;
  *negative_g = SAT - *colored_g;
  *negative_b = SAT - *colored_b;
}

#endif

void process_transform(const void *p, int size)
{
  int i, newi;
  unsigned char *pptr = (unsigned char *)p;
  
  if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
  {
    if(transform_on_off == 1)
    {
      for(i=0, newi=0; i<size; i=i+4, newi=newi+6)
      {
        negative_transformation((int)pptr[newi], (int)pptr[newi+1], (int)pptr[newi+2],&negativebuffer[newi], &negativebuffer[newi+1], &negativebuffer[newi+2]);
        negative_transformation((int)pptr[newi+3], (int)pptr[newi+4], (int)pptr[newi+5],&negativebuffer[newi+3], &negativebuffer[newi+4], &negativebuffer[newi+5]);
      }
    }
    else if(transform_on_off == 0)
    {
      memcpy((void *)&(negativebuffer[0]),p,sizeof(negativebuffer));
    }
    else
    {
      /* Do nothing */
    }
  }
  else
  {
    /* Do nothing */
  }
}



void process_image(const void *p, int size)
{
  int i, newi;

  g_framesize = size;
  int y_temp, y2_temp, u_temp, v_temp;
  unsigned char *pptr = (unsigned char *)p;

  //record when process was called
 
  syslog(LOG_INFO,"frame %d: ",framecnt);
    
#ifdef DUMP_FRAMES	

  if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
  {
    #if COLOR_CONVERT_RGB 
    // Pixels are YU and YV alternating, so YUYV which is 4 bytes
    // We want RGB, so RGBRGB which is 6 bytes


    #if UNPROCESSED_FRAMES
    
    for(i=0, newi=0; i<size; i=i+4, newi=newi+6)
    {
      y_temp=(int)pptr[i]; u_temp=(int)pptr[i+1]; y2_temp=(int)pptr[i+2]; v_temp=(int)pptr[i+3];     
      yuv2rgb(y_temp, u_temp, v_temp, &bigbuffer[newi], &bigbuffer[newi+1], &bigbuffer[newi+2]);
      yuv2rgb(y2_temp, u_temp, v_temp, &bigbuffer[newi+3], &bigbuffer[newi+4], &bigbuffer[newi+5]);
      
      //negative_transformation(&bigbuffer[newi], &bigbuffer[newi+1], &bigbuffer[newi+2],&negativebuffer[newi], &negativebuffer[newi+1], &negativebuffer[newi+2]);
      //negative_transformation(&bigbuffer[newi+3], &bigbuffer[newi+4], &bigbuffer[newi+5],&negativebuffer[newi+3], &negativebuffer[newi+4], &negativebuffer[newi+5]);
    }
    
    
    
    
    
    #if 0
    if(transform_on_off == 1)
    {
      for(i=0, newi=0; i<size; i=i+4, newi=newi+6)
      {
        y_temp=(int)pptr[i]; u_temp=(int)pptr[i+1]; y2_temp=(int)pptr[i+2]; v_temp=(int)pptr[i+3];     
        yuv2rgb(y_temp, u_temp, v_temp, &bigbuffer[newi], &bigbuffer[newi+1], &bigbuffer[newi+2]);
        yuv2rgb(y2_temp, u_temp, v_temp, &bigbuffer[newi+3], &bigbuffer[newi+4], &bigbuffer[newi+5]);
        
        //negative_transformation(&bigbuffer[newi], &bigbuffer[newi+1], &bigbuffer[newi+2],&negativebuffer[newi], &negativebuffer[newi+1], &negativebuffer[newi+2]);
        //negative_transformation(&bigbuffer[newi+3], &bigbuffer[newi+4], &bigbuffer[newi+5],&negativebuffer[newi+3], &negativebuffer[newi+4], &negativebuffer[newi+5]);
      }
        
    }
    else if(transform_on_off == 0)
    {
    
      for(i=0, newi=0; i<size; i=i+4, newi=newi+6)
      {
        y_temp=(int)pptr[i]; u_temp=(int)pptr[i+1]; y2_temp=(int)pptr[i+2]; v_temp=(int)pptr[i+3];     
        yuv2rgb(y_temp, u_temp, v_temp, &bigbuffer[newi], &bigbuffer[newi+1], &bigbuffer[newi+2]);
        yuv2rgb(y2_temp, u_temp, v_temp, &bigbuffer[newi+3], &bigbuffer[newi+4], &bigbuffer[newi+5]);
      }
      
      //memcpy(negativebuffer,bigbuffer,sizeof(bigbuffer));
    }
    else
    {
      /* do nothing */
    }
    #endif
    #else
    
    for(i=0, newi=0; i<size; i=i+4, newi=newi+6)
    {
      printf("Does not execute\n");
      y_temp=(int)pptr[i]; u_temp=(int)pptr[i+1]; y2_temp=(int)pptr[i+2]; v_temp=(int)pptr[i+3];
      yuv2rgb(y_temp, u_temp, v_temp, &bigbuffer[newi], &bigbuffer[newi+1], &bigbuffer[newi+2]);
      yuv2rgb(y2_temp, u_temp, v_temp, &bigbuffer[newi+3], &bigbuffer[newi+4], &bigbuffer[newi+5]);
      
      negative_transformation(&bigbuffer[newi], &bigbuffer[newi+1], &bigbuffer[newi+2],&negativebuffer[newi], &negativebuffer[newi+1], &negativebuffer[newi+2]);
      negative_transformation(&bigbuffer[newi+3], &bigbuffer[newi+4], &bigbuffer[newi+5],&negativebuffer[newi+3], &negativebuffer[newi+4], &negativebuffer[newi+5]);
    }   
       
    #endif



    if(framecnt > -1) 
    {
      #if UNPROCESSED_FRAMES      
      g_framesize = size;
      //printf("unprocessed frame_read frame\n");
     // //dump_ppm(bigbuffer, ((size*6)/4), framecnt, &frame_time);
      #else
      //printf("processed frame_read frame\n");
      g_framesize = size;
      #endif
    }
    #else

    // Pixels are YU and YV alternating, so YUYV which is 4 bytes
    // We want Y, so YY which is 2 bytes
    g_framesize = size;
    
    if(framecnt > -1) 
    {
      clock_gettime(CLOCK_MONOTONIC, &ts_transform_start);
    }
    
    for(i=0, newi=0; i<size; i=i+4, newi=newi+2)
    {
      // Y1=first byte and Y2=third byte
      bigbuffer[newi]=pptr[i];
      bigbuffer[newi+1]=pptr[i+2];
    }

    if(framecnt > -1)
    {
      clock_gettime(CLOCK_MONOTONIC, &ts_transform_stop);
      transform_time[framecnt] = dTime(ts_transform_stop, ts_transform_start);
      syslog(LOG_INFO, "transform_time individual is %lf\n", transform_time[framecnt]);
      //acq_to_tranform_time[framecnt] = dTime(ts_transform_stop, ts_read_capture_start);
      //syslog(LOG_INFO, "acq_to_tranform_time individual is %lf\n", acq_to_tranform_time[framecnt]);
    }
    #endif
  }
  
  // This just dumps the frame to a file now, but you could replace with whatever image
  // processing you wish.
  else if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_GREY)
  {
    syslog(LOG_INFO, "Dump graymap as-is size %d\n", size);
    dump_pgm(p, size, framecnt, &frame_time);
  }
  else
  {
    syslog(LOG_INFO, "ERROR - unknown dump format\n");
  }
  
#endif
}


void yuv2rgb(int y, int u, int v, unsigned char *r, unsigned char *g, unsigned char *b)
{
  int r1, g1, b1;

  // replaces floating point coefficients
  int c = y-16, d = u - 128, e = v - 128;       

  // Conversion that avoids floating point
  r1 = (298 * c           + 409 * e + 128) >> 8;
  g1 = (298 * c - 100 * d - 208 * e + 128) >> 8;
  b1 = (298 * c + 516 * d           + 128) >> 8;

  // Computed values may need clipping.
  if (r1 > 255) r1 = 255;
  if (g1 > 255) g1 = 255;
  if (b1 > 255) b1 = 255;

  if (r1 < 0) r1 = 0;
  if (g1 < 0) g1 = 0;
  if (b1 < 0) b1 = 0;

  *r = r1 ;
  *g = g1 ;
  *b = b1 ;
}



char ppm_header[137]="P6\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
char ppm_dumpname[21]="frames/orig0000.ppm";

void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time)
{
  int written, total, dumpfd;

  /* 11th number is a test number. 4 digit number is updated here. */
  snprintf(ppm_dumpname+11, 9, "%04d", tag);
  /* PPM is appended in the file name */
  strncat(ppm_dumpname+15, ".ppm", 5);

  dumpfd = open(ppm_dumpname, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);
  
  snprintf(ppm_header+4, 11, "%010d", (int)time->tv_sec);
  strcat(ppm_header+14, " sec ");
  snprintf(ppm_header+19, 11, "%010d", (int)((time->tv_nsec)/1000000));
  strcat(ppm_header+29, " msec \n");
  strcat(ppm_header+36, ppm_uname_string);
  strcat(ppm_header+122, ""HRES_STR" "VRES_STR"\n255\n");


  
  // subtract 1 from sizeof header because it includes the null terminator for the string
  written=write(dumpfd, ppm_header, sizeof(ppm_header)-1);

  total=0;
  
  clock_gettime(CLOCK_MONOTONIC, &ts_writeback_stop);
  writeback_time[framecnt] = dTime(ts_writeback_stop, ts_writeback_start);
  syslog(LOG_INFO, "writeback_time individual is %lf\n", writeback_time[framecnt]);

  do
  {
    written=write(dumpfd, p, size);
    total+=written;
  }while(total < size);

  clock_gettime(CLOCK_MONOTONIC, &time_now);
  fnow = (double)time_now.tv_sec + (double)time_now.tv_nsec / NANOSEC_PER_SEC;
  syslog(LOG_INFO, "Frame_written_to_flash %d at %lf %d bytes\n",framecnt, (fnow-fstart), total);

  close(dumpfd);
    
}



char pgm_header[137]="P5\n#9999999999 sec 9999999999 msec \n"HRES_STR" "VRES_STR"\n255\n";
char pgm_dumpname[21]="frames/bgw_0000.pgm";

void dump_pgm(const void *p, int size, unsigned int tag, struct timespec *time)
{
  int written, total, dumpfd;

  snprintf(&pgm_dumpname[11], 9, "%04d", tag);
  strncat(&pgm_dumpname[15], ".pgm", 5);
  dumpfd = open(pgm_dumpname, O_WRONLY | O_NONBLOCK | O_CREAT, 00666);

  snprintf(&pgm_header[4], 11, "%010d", (int)time->tv_sec);
  strcat(&pgm_header[14], " sec ");
  snprintf(&pgm_header[19], 11, "%010d", (int)((time->tv_nsec)/1000000));

  strcat(pgm_header+29, " msec \n");
  strcat(pgm_header+36, ppm_uname_string);
  strcat(pgm_header+122, ""HRES_STR" "VRES_STR"\n255\n");

  // subtract 1 from sizeof header because it includes the null terminator for the string
  written=write(dumpfd, pgm_header, sizeof(pgm_header)-1);

  total=0;

  do
  {
    written=write(dumpfd, p, size);
    total+=written;
  }while(total < size);

  clock_gettime(CLOCK_MONOTONIC, &time_now);
  fnow = (double)time_now.tv_sec + (double)time_now.tv_nsec / NANOSEC_PER_SEC;
  //syslog(LOG_INFO, "Frame written to flash at %lf, %d, bytes\n", (fnow-fstart), total);

  close(dumpfd);
    
}

void print_analysis()
{
  int n = 2;
  int start = 3;
  int z =2;
  
  printf("Entered printing mode");
  /* read/capture time */
  double sum = 0;
  double avg_time = 0;
  double temp_value = read_capture_time[n];
  /* 1801 frames -1 required */
  for(int i = start; i<BUFF_LENGTH-1;i++ )
  {
    if(temp_value < read_capture_time[i])
    {
      temp_value = read_capture_time[i];
    }
    //printf("read_capture_time:: %lf\n",read_capture_time[i]);
    sum = sum + read_capture_time[i];
  }
  avg_time = sum/(CAPTURE_FRAMES-z);
  syslog(LOG_INFO, "Total frames = %d frames, Average capture time = %lf sec, Average read_capture_time Frame rate = %lf FPS\n",CAPTURE_FRAMES, (double)avg_time, ((double)(1/avg_time)));
  syslog(LOG_INFO, "WCET - read_capture_time %lf sec and %lf FPS\n",temp_value, (1/temp_value));
  
  /* transform time */
  sum = 0;
  avg_time = 0;
  temp_value = transform_time[n];
  for(int i = start; i<BUFF_LENGTH;i++ )
  {
    if(temp_value < transform_time[i])
    {
      temp_value = transform_time[i];
    }
    //printf("transform_time:: %lf\n",transform_time[i]);
    sum = sum + transform_time[i];
  }
  avg_time = sum/(CAPTURE_FRAMES-z);
  
  syslog(LOG_INFO, "Total frames = %d frames, Average transform time = %lf sec, Average Transform Frame rate = %lf FPS\n",CAPTURE_FRAMES, (double)avg_time, ((double)(1/avg_time)));
  syslog(LOG_INFO, "WCET - transform_time %lf sec and FPS is %lf FPS\n",temp_value, (1/temp_value));
  
  /* write-back time */
  sum = 0;
  avg_time = 0;
  temp_value = writeback_time[n];
  
  for(int i = start; i<BUFF_LENGTH;i++ )
  {
    if(temp_value < writeback_time[i])
    {
      temp_value = writeback_time[i];
    }
    //printf("writeback_time:: %lf\n",writeback_time[i]);
    sum = sum + writeback_time[i];
  }
  avg_time = sum/(CAPTURE_FRAMES-z);
  
  syslog(LOG_INFO, "Total frames = %d frames, Average writeback time = %lf sec, Average writeback Frame rate = %lf FPS\n",CAPTURE_FRAMES, (double)avg_time, ((double)(1/avg_time)));
  syslog(LOG_INFO, "WCET - writeback_time %lf sec and FPS is %lf FPS\n",temp_value, (1/temp_value));
  
  /* end_to_end_time time */
  sum = 0;
  avg_time = 0;
  temp_value = negative_transformation_time[n];
  for(int i = start; i<BUFF_LENGTH;i++ )
  {
    if(temp_value < negative_transformation_time[i])
    {
      temp_value = negative_transformation_time[i];
    }
    //printf("end_to_end_time:: %lf\n",end_to_end_time[i]);
    sum = sum + negative_transformation_time[i];
  }
  avg_time = sum/(CAPTURE_FRAMES-z);
  
  syslog(LOG_INFO, "Total frames = %d frames, Average negative_transformation_time time = %lf sec, Average negative_transformation_time Frame rate = %lf FPS\n",CAPTURE_FRAMES, (double)avg_time, ((double)(1/avg_time)));
  syslog(LOG_INFO, "WCET - negative_transformation_time %lf sec and FPS is %lf FPS\n",temp_value, (1/temp_value));
  
  #if 0
  sum = 0;
  avg_time = 0;
  temp_value = acq_to_tranform_time[n];
  for(int i = start; i<BUFF_LENGTH;i++ )
  {
    if(temp_value < acq_to_tranform_time[i])
    {
      temp_value = acq_to_tranform_time[i];
    }
    //printf("writeback_time:: %lf\n",writeback_time[i]);
    sum = sum + acq_to_tranform_time[i];
  }
  avg_time = sum/(CAPTURE_FRAMES-z);
  
  syslog(LOG_INFO, "Total frames = %d frames, Average acq_to_tranform_time time = %lf sec, Average acq_to_tranform_time Frame rate = %lf FPS\n",CAPTURE_FRAMES, (double)avg_time, ((double)(1/avg_time)));
  syslog(LOG_INFO, "WCET - acq_to_tranform_time %lf sec and FPS is %lf FPS\n",temp_value, (1/temp_value));
  #endif

}

void start_capturing(void)
{
  unsigned int i;
  enum v4l2_buf_type type;

  switch (io) 
  {
    case IO_METHOD_MMAP:
    {
      for(i = 0; i < n_buffers; ++i) 
      {
        syslog(LOG_INFO, "allocated buffer %d\n", i);
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
      }
      
      type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      if(-1 == xioctl(fd, VIDIOC_STREAMON, &type))
              errno_exit("VIDIOC_STREAMON");
      break;
    }
    case IO_METHOD_USERPTR:
    {
      for (i = 0; i < n_buffers; ++i)
      {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type      =   V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory    =   V4L2_MEMORY_USERPTR;
        buf.index     =   i;
        buf.m.userptr =   (unsigned long)buffers[i].start;
        buf.length    =   buffers[i].length;

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
      }
      
      type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
              errno_exit("VIDIOC_STREAMON");
      break;
    }
    case IO_METHOD_READ:
    {
      /* Nothing to do. */
      break;
    }
  }
}


void stop_capturing(void)
{
  enum v4l2_buf_type type;

  switch (io)
  {
    case IO_METHOD_READ:
    {
      /* Nothing to do. */
      break;
    }
    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
    {
      type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
              errno_exit("VIDIOC_STREAMOFF");
      break;
    }
  }
  
  if(framecnt > -1)
  {	
    clock_gettime(CLOCK_MONOTONIC, &time_stop);
    fstop = (double)time_stop.tv_sec + (double)time_stop.tv_nsec / NANOSEC_PER_SEC;
  }
}



void init_userp(unsigned int buffer_size)
{
  struct v4l2_requestbuffers req;

  CLEAR(req);

  req.count  = 4;
  req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_USERPTR;

  if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
    if (EINVAL == errno)
    {
      fprintf(stderr, "%s does not support "
               "user pointer i/o\n", dev_name);
      exit(EXIT_FAILURE);
    }
    else
    {
      errno_exit("VIDIOC_REQBUFS");
    }
  }

  buffers = calloc(4, sizeof(*buffers));

  if (!buffers)
  {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }

  for (n_buffers = 0; n_buffers < 4; ++n_buffers)
  {
    buffers[n_buffers].length = buffer_size;
    buffers[n_buffers].start = malloc(buffer_size);

    if(!buffers[n_buffers].start)
    {
      fprintf(stderr, "Out of memory\n");
      exit(EXIT_FAILURE);
    }
  }
}



void init_mmap(void)
{
  struct v4l2_requestbuffers req;

  CLEAR(req);

  req.count = DRIVER_MMAP_BUFFERS;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if(-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) 
  {
    if (EINVAL == errno) 
    {
      fprintf(stderr, "%s does not support "
      "memory mapping\n", dev_name);
      exit(EXIT_FAILURE);
    }
    else 
    {
      errno_exit("VIDIOC_REQBUFS");
    }
  }

  if(req.count < 2) 
  {
    fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
    exit(EXIT_FAILURE);
  }

  buffers = calloc(req.count, sizeof(*buffers));

  if(!buffers) 
  {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }

  for(n_buffers = 0; n_buffers < req.count; ++n_buffers)
  {
    struct v4l2_buffer buf;

    CLEAR(buf);

    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index  = n_buffers;

    if(-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
        errno_exit("VIDIOC_QUERYBUF");

    buffers[n_buffers].length = buf.length;
    buffers[n_buffers].start = mmap(NULL /* start anywhere */,
                                    buf.length,
                                    PROT_READ | PROT_WRITE /* required */,
                                    MAP_SHARED /* recommended */,
                                    fd,
                                    buf.m.offset);

    if(MAP_FAILED == buffers[n_buffers].start)
            errno_exit("mmap");
  }
}



void init_read(unsigned int buffer_size)
{
  buffers = calloc(1, sizeof(*buffers));
  
  if(!buffers) 
  {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }

  buffers[0].length = buffer_size;
  buffers[0].start = malloc(buffer_size);

  if(!buffers[0].start) 
  {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }
}


int xioctl(int fh, int request, void *arg)
{
  int r;

  do 
  {
    r = ioctl(fh, request, arg);

  }while (-1 == r && EINTR == errno);

  return r;
}


void init_device(void)
{
  struct v4l2_capability cap;
  struct v4l2_cropcap cropcap;
  struct v4l2_crop crop;
  unsigned int min;

  if(-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap))
  {
    if (EINVAL == errno)
    {
      fprintf(stderr, "%s is no V4L2 device\n",
               dev_name);
      exit(EXIT_FAILURE);
    }
    else
    {
      errno_exit("VIDIOC_QUERYCAP");
    }
  }

  if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
  {
    fprintf(stderr, "%s is no video capture device\n",dev_name);
    exit(EXIT_FAILURE);
  }

  switch (io)
  {
    case IO_METHOD_READ:
      if(!(cap.capabilities & V4L2_CAP_READWRITE))
      {
          fprintf(stderr, "%s does not support read i/o\n",dev_name);
          exit(EXIT_FAILURE);
      }
      break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
      if(!(cap.capabilities & V4L2_CAP_STREAMING))
      {
          fprintf(stderr, "%s does not support streaming i/o\n",dev_name);
          exit(EXIT_FAILURE);
      }
      break;
  }

  /* Select video input, video standard and tune here. */
  CLEAR(cropcap);

  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if(0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap))
  {
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect; /* reset to default */

    if(-1 == xioctl(fd, VIDIOC_S_CROP, &crop))
    {
      switch (errno)
      {
        case EINVAL:
        {
          /* Cropping not supported. */
          break;
        }
        default:
        {
          /* Errors ignored. */
          break;
        }
      }
    }
  }
  else
  {
    /* Errors ignored. */
  }


  CLEAR(fmt);

  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if(force_format)
  {
    syslog(LOG_INFO, "FORCING FORMAT\n");
    fmt.fmt.pix.width       = HRES;
    fmt.fmt.pix.height      = VRES;

    // Specify the Pixel Coding Format here

    // This one works for Logitech C200
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
            errno_exit("VIDIOC_S_FMT");

      /* Note VIDIOC_S_FMT may change width and height. */
  }
  else
  {
    syslog(LOG_INFO, "ASSUMING FORMAT\n");
    /* Preserve original settings as set by v4l2-ctl for example */
    if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
          errno_exit("VIDIOC_G_FMT");
  }

  /* Buggy driver paranoia. */
  min = fmt.fmt.pix.width * 2;
  if (fmt.fmt.pix.bytesperline < min)
          fmt.fmt.pix.bytesperline = min;
  min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
  if (fmt.fmt.pix.sizeimage < min)
          fmt.fmt.pix.sizeimage = min;

  switch(io)
  {
    case IO_METHOD_READ:
    {
      init_read(fmt.fmt.pix.sizeimage);
      break;
    }
    case IO_METHOD_MMAP:
    {
      init_mmap();
      break;
    }
    case IO_METHOD_USERPTR:
    {
      init_userp(fmt.fmt.pix.sizeimage);
      break;
    }
  }
}


void uninit_device(void)
{
  unsigned int i;

  switch(io) 
  {
    case IO_METHOD_READ:
    {
      free(buffers[0].start);
      break;
    }

    case IO_METHOD_MMAP:
    {
      for(i = 0; i < n_buffers; ++i)
              if (-1 == munmap(buffers[i].start, buffers[i].length))
                      errno_exit("munmap");
      break;
    }

    case IO_METHOD_USERPTR:
    {
      for(i = 0; i < n_buffers; ++i)
              free(buffers[i].start);
      break;
    }
  }

  free(buffers);
}


void open_device(void)
{
  struct stat st;

  /* stat() stats the file pointed to by path and fills in buf. */
  if(-1 == stat(dev_name, &st))
  {
    fprintf(stderr, "Cannot identify '%s': %d, %s\n",
      dev_name, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  /* char special */
  if(!S_ISCHR(st.st_mode))
  {
    fprintf(stderr, "%s is no device\n", dev_name);
    exit(EXIT_FAILURE);
  }

  fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

  if(-1 == fd)
  {
    fprintf(stderr, "Cannot open '%s': %d, %s\n",
      dev_name, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void close_device(void)
{
  if(-1 == close(fd))
    errno_exit("close");

  fd = -1;
}

void v4l2_frame_acquisition_initialization()
{
  open_device();
  init_device();
  start_capturing();
}

void v4l2_frame_acquisition_shutdown()
{
  stop_capturing();
  syslog(LOG_INFO, "Total capture time=%lf, for %d frames, %lf FPS\n", (fstop-fstart), CAPTURE_FRAMES, ((double)CAPTURE_FRAMES / (fstop-fstart)));
  uninit_device();
  close_device();
}

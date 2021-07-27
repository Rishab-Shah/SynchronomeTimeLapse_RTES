#ifndef _CBFIFO_H_
#define _CBFIFO_H_
/*******************************************************************************
Header files
*******************************************************************************/
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

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
/*******************************************************************************
Macros
*******************************************************************************/
#define CBFIFO_SIZE             		(32)
/*******************************************************************************
Structures
*******************************************************************************/
typedef struct sCircularBuffer
{
    uint8_t cbBuf[CBFIFO_SIZE];       /* Circular buffer in place */
    uint16_t size;                    /* Size in power of 2 */
    uint16_t read;                    /* Read position 0 to size-1 */
    uint16_t write;                   /* Write position 0 to size-1 */
    int16_t tracker;                  /* To track the position */
}CB_t;
/*******************************************************************************
Function declarations
*******************************************************************************/
void cbfifo_init(CB_t *cbfifo , size_t size);
size_t cbfifo_enqueue(CB_t *cbfifo,void *buf, size_t nbyte);
size_t cbfifo_dequeue(CB_t *cbfifo,void *buf, size_t nbyte);
size_t cbfifo_length(CB_t *cbfifo);
size_t cbfifo_capacity(CB_t *cbfifo);
void cbfifo_dump_state(CB_t *cbfifo);
void DiagnosticMessage(CB_t *cbfifo);
int cbfifo_full(CB_t *cbfifo);


#endif // _CBFIFO_H_
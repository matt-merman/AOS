#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/string.h>	
#include <linux/errno.h>
#include <linux/tty.h>		/* For the tty declarations */
#include <linux/version.h>	/* For LINUX_VERSION_CODE */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mattia Di Battista");
MODULE_DESCRIPTION("Multi-flow device file");

#define MODNAME "MULTI-FLOW"

#define DEVICE_NAME "my-new-dev"  /* Device file name in /dev/ - not mandatory  */

//#define OBJECT_MAX_SIZE  (4096) //just one page
#define MINORS 128
#define AUDIT if(1)

#define NUM_FLOW 2

#define LOW_PRIORITY 0
#define HIGH_PRIORITY 1
#define BLOCKING 0
#define NON_BLOCKING 1

//#define SINGLE_INSTANCE //just one session at a time across all I/O node
//#define SINGLE_SESSION_OBJECT //just one session per I/O node at a time

#ifndef _INFOH_
#define _INFOH_

typedef struct _memory_node
{
        char *buffer;
        struct _memory_node *next;
} memory_node;

typedef struct _object_state
{
#ifdef SINGLE_SESSION_OBJECT
        struct mutex object_busy;
#endif
        struct mutex operation_synchronizer;
        // int valid_bytes;
        // char * stream_content;//the I/O node is a buffer in memory
        memory_node *head; // head to al written buffer
        wait_queue_head_t wq;
} object_state;

typedef struct _session
{

        bool priority;         // priority level (high or low) for the operations
        bool blocking;         // blocking vs non-blocking read and write operations
        unsigned long timeout; // setup of a timeout regulating the awake of blocking operations

} session;

#endif

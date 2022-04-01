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
#include <linux/tty.h>     /* For the tty declarations */
#include <linux/version.h> /* For LINUX_VERSION_CODE */
#include <linux/moduleparam.h>
#include <linux/jiffies.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mattia Di Battista");
MODULE_DESCRIPTION("Multi-flow device file");

#define MODNAME "MULTI-FLOW"
#define DEVICE_NAME "test" /* Device file name in /dev/ - not mandatory  */

#define MINORS 128
#define AUDIT if (1)
#define NUM_FLOW 2

#define LOW_PRIORITY 0
#define HIGH_PRIORITY 1
#define BLOCKING 0
#define NON_BLOCKING 1

//#define TEST //macro used to unrelease the lock on write op. In this way you can test blocking operations.

#ifndef _INFOH_
#define _INFOH_

static int enabled_device[MINORS];
module_param_array(enabled_device, int, NULL, 0660);
MODULE_PARM_DESC(enabled_device, "Module parameter is implemented in order to enable or disable " \
"the device file, in terms of a specific minor number. If it is disabled, " \
"any attempt to open a session should fail (but already open sessions will be still managed).");

static int hp_bytes[MINORS];
module_param_array(hp_bytes, int, NULL, 0660);
MODULE_PARM_DESC(hp_bytes, "Number of bytes currently present in the high priority flow.");

static int lp_bytes[MINORS];
module_param_array(lp_bytes, int, NULL, 0660);
MODULE_PARM_DESC(lp_bytes, "Number of bytes currently present in the low priority flow.");

static int hp_threads[MINORS];
module_param_array(hp_threads, int, NULL, 0660);
MODULE_PARM_DESC(hp_threads, "Number of threads currently waiting for data along the high priority flow.");

static int lp_threads[MINORS];
module_param_array(lp_threads, int, NULL, 0660);
MODULE_PARM_DESC(lp_threads, "Number of threads currently waiting for data along the low priority flow.");

typedef struct _memory_node
{
        char *buffer;
        struct _memory_node *next;
} memory_node;

typedef struct _object_state
{
        struct mutex operation_synchronizer;
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

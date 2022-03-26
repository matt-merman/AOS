#include "info.h"
#include <asm-generic/param.h>

static int blocking(unsigned long timeout, struct mutex *mutex, wait_queue_head_t *wq)
{
   if (timeout == 0) return 0;

   AUDIT printk("%s: thread %d going to usleep for %lu microsecs\n", MODNAME, current->pid, timeout);

   timeout = (timeout*HZ)/1000;

   //Returns 0, if the condition evaluated to false after the timeout elapsed
   int val = wait_event_timeout(*wq, mutex_trylock(mutex), timeout);
   
   AUDIT printk("%s: thread %d go out from usleep\n", MODNAME, current->pid);
   
   if(!val) return 0;

   return 1;
}
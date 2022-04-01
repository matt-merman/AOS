#include "info.h"
#include <asm-generic/param.h>

static int blocking(unsigned long, struct mutex *, wait_queue_head_t *);
static wait_queue_head_t * get_lock(object_state *, session *, int);

#ifndef _COMMONH_
#define _COMMONH_

static int blocking(unsigned long timeout, struct mutex *mutex, wait_queue_head_t *wq)
{
   int val;

   if (timeout == 0) return 0;

   AUDIT printk("%s: thread %d going to sleep for %lu microsecs\n", MODNAME, current->pid, timeout);

   timeout = (timeout*HZ)/1000;   

   //Returns 0, if the condition evaluated to false after the timeout elapsed
   val = wait_event_timeout(*wq, mutex_trylock(mutex), timeout);
   
   AUDIT printk("%s: thread %d go out from sleep\n", MODNAME, current->pid);
   
   if(!val) return 0;

   return 1;
}

static wait_queue_head_t * get_lock(object_state *the_object, session *session, int minor){

   int ret;
   wait_queue_head_t *wq;
   wq = &the_object->wq;
   
   // Try to acquire the mutex atomically.
   // Returns 1 if the mutex has been acquired successfully,
   // and 0 on contention.
   ret = mutex_trylock(&(the_object->operation_synchronizer));
   if (!ret)
   {

      printk("%s: unable to get lock now\n", MODNAME);
      if (session->blocking == BLOCKING)
      {

         if(session->priority == HIGH_PRIORITY) hp_threads[minor] ++;
         else lp_threads[minor] ++;

         ret = blocking(session->timeout, &the_object->operation_synchronizer, wq);
         
         if(session->priority == HIGH_PRIORITY) hp_threads[minor] --;
         else lp_threads[minor] --;

         if (ret == 0) return NULL;
      }
      else
         return NULL;
   }

   return wq;
}

#endif

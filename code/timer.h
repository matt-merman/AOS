#include "info.h"
#include <asm-generic/param.h>

typedef struct _control_record
{
   struct task_struct *task;
   int pid;
   int awake;
   struct hrtimer hr_timer;
} control_record;

static int blocking(unsigned long timeout, struct mutex *mutex, wait_queue_head_t *wq)
{
   control_record data, *control;
   ktime_t ktime_interval;

   if (timeout == 0)
      return 0;

   control = &data; // set the pointer to the current stack area

   AUDIT printk("%s: thread %d going to usleep for %lu microsecs\n", MODNAME, current->pid, timeout);

   ktime_interval = ktime_set(0, timeout * 1000);

   control->task = current;
   control->pid = current->pid;
   control->awake = 0;

   bool condition = mutex_trylock(mutex) == 1;
   wait_event_timeout(*wq, condition, ktime_interval);

   return 0;
}
/*
source file usleep.c
*/

#include "info.h"

#define NO (0)
#define YES (NO+1)

typedef struct _control_record{
        struct task_struct *task;       
        int pid;
        int awake;
        struct hrtimer hr_timer;
} control_record;

static enum hrtimer_restart my_hrtimer_callback( struct hrtimer *timer ){

        control_record *control;
        struct task_struct *the_task;

        control = (control_record*)container_of(timer,control_record, hr_timer);
        control->awake = YES;
        the_task = control->task;
        wake_up_process(the_task);

        return HRTIMER_NORESTART;
}

static int blocking(unsigned long timeout){

   unsigned long microsecs = timeout;

   control_record data;
   control_record* control;
   ktime_t ktime_interval;
   DECLARE_WAIT_QUEUE_HEAD(the_queue);//here we use a private queue - wakeup is selective via wake_up_process

   if(microsecs == 0) return 0;

	control = &data;//set the pointer to the current stack area

   AUDIT printk("%s: thread %d going to usleep for %lu microsecs\n",MODNAME,current->pid,microsecs);

   ktime_interval = ktime_set( 0, microsecs*1000 );

   control->task = current;
   control->pid  = current->pid;
   control->awake = NO;

   hrtimer_init(&(control->hr_timer), CLOCK_MONOTONIC, HRTIMER_MODE_REL);

   control->hr_timer.function = &my_hrtimer_callback;
   hrtimer_start(&(control->hr_timer), ktime_interval, HRTIMER_MODE_REL);

   wait_event_interruptible(the_queue, control->awake == YES);

   hrtimer_cancel(&(control->hr_timer));
   
   AUDIT printk("%s: thread %d exiting usleep\n",MODNAME, current->pid);

	if(unlikely(control->awake != YES)) return -1;

   return 0;
}
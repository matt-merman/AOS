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

#include "info.h"

typedef struct _packed_work{
        void* buffer;
        long code;
        struct work_struct the_work;
} packed_work;


void audit(unsigned long data){

        printk("%s: ------------------------------\n",MODNAME);
        printk("%s: this print comes from kworker daemon with PID=%d - running on CPU-core %d\n",MODNAME,current->pid,smp_processor_id());
        
        printk("%s: running task with code  %ld\n",MODNAME,container_of((void*)data,packed_work,the_work)->code);

        
        printk("%s: releasing the task buffer at address %p - container of task is at %p\n",MODNAME,(void*)data,container_of((void*)data,packed_work,the_work));

        kfree((void*)container_of((void*)data,packed_work,the_work));

        module_put(THIS_MODULE);

}

long put_work(int request_code){

        packed_work *the_task;

        if(!try_module_get(THIS_MODULE)) return -ENODEV;

        printk("%s: requested deferred work with request code: %d\n",MODNAME,request_code);
        
        the_task = kzalloc(sizeof(packed_work),GFP_ATOMIC);//non blocking memory allocation

        if (the_task == NULL) {
                printk("%s: tasklet buffer allocation failure\n",MODNAME);
                module_put(THIS_MODULE);
                return -1;
        }

        the_task->buffer = the_task;
        the_task->code = request_code;

        printk("%s: work buffer allocation success - address is %p\n",MODNAME,the_task);

        __INIT_WORK(&(the_task->the_work),(void*)audit,(unsigned long)(&(the_task->the_work)));

        schedule_work(&the_task->the_work);

        return 0;
}
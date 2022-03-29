#include "common.h"

int write(object_state *, const char *, loff_t *, size_t, session *, int);
void delayed_write(unsigned long);
long put_work(struct file *, const char *, size_t, loff_t *, object_state *, session *, int);

typedef struct _packed_work
{
        struct work_struct the_work;
        struct file *filp;
        const char *buff;
        size_t len;
        loff_t *off;
        object_state *the_object;
        session *session;
        int minor;
} packed_work;

int write(object_state *the_object,
          const char *buff,
          loff_t *off, 
          size_t len, 
          session *session,
          int minor)
{

        memory_node *node, *current_node;
        char *buffer;
        int ret;
        wait_queue_head_t *wq;
        
        wq = get_lock(the_object, session, minor);
        if (wq == NULL) return 0;

        if(session->priority == HIGH_PRIORITY) hp_bytes[minor] += len;
        else lp_bytes[minor] += len;

        node = kmalloc(sizeof(memory_node), GFP_KERNEL);
        buffer = kmalloc(len, GFP_KERNEL);
        if (node == NULL || buffer == NULL)
        {
                printk("%s: unable to allocate a memory\n", MODNAME);
                mutex_unlock(&(the_object->operation_synchronizer));
                wake_up(wq);
                return -1;
        }

        AUDIT printk("%s: ALLOCATED a new memory node\n", MODNAME);
        AUDIT printk("%s: ALLOCATED %ld bytes\n", MODNAME, len);

        current_node = the_object->head;

        while (current_node->next != NULL)
                current_node = current_node->next;

        current_node->buffer = buffer;
        current_node->next = node;

        node->next = NULL;
        node->buffer = NULL;

        // returns the number of bytes NOT copied
        ret = copy_from_user(current_node->buffer, buff, len);

        //*off += (len - ret);
        *off = 0;

        //TEST blocking      
        mutex_unlock(&(the_object->operation_synchronizer));
        wake_up(wq);

        return len - ret;
}

void delayed_write(unsigned long data)
{
        session *session = container_of((void *)data, packed_work, the_work)->session;
        int minor = container_of((void *)data, packed_work, the_work)->minor;

        size_t len = container_of((void *)data, packed_work, the_work)->len;
        loff_t *off = container_of((void *)data, packed_work, the_work)->off;
        object_state *the_object = container_of((void *)data, packed_work, the_work)->the_object;
        const char *buff = container_of((void *)data, packed_work, the_work)->buff;

        AUDIT printk("%s: this print comes from kworker daemon with PID=%d - running on CPU-core %d\n", MODNAME, current->pid, smp_processor_id());

        write(the_object, buff, off, len, session, minor);

        AUDIT printk("%s: releasing the task buffer at address %p - container of task is at %p\n", MODNAME, (void *)data, container_of((void *)data, packed_work, the_work));

        kfree((void *)container_of((void *)data, packed_work, the_work));

        module_put(THIS_MODULE);
}

long put_work(struct file *filp,
              const char *buff,
              size_t len,
              loff_t *off,
              object_state *the_object,
              session *session,
              int minor)
{

        packed_work *the_task;

        if (!try_module_get(THIS_MODULE))
                return -ENODEV;

        AUDIT printk("%s: requested deferred work\n", MODNAME);

        the_task = kzalloc(sizeof(packed_work), GFP_ATOMIC); // non blocking memory allocation

        if (the_task == NULL)
        {
                printk("%s: tasklet buffer allocation failure\n", MODNAME);
                module_put(THIS_MODULE);
                return -1;
        }

        the_task->filp = filp;
        the_task->buff = buff;
        the_task->len = len;
        the_task->off = off;
        the_task->the_object = the_object;
        the_task->session = session;
        the_task->minor = minor;


        AUDIT printk("%s: work buffer allocation success - address is %p\n", MODNAME, the_task);

        __INIT_WORK(&(the_task->the_work), (void *)delayed_write, (unsigned long)(&(the_task->the_work)));

        schedule_work(&the_task->the_work);

        return 0;
}
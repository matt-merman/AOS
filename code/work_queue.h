#include "info.h"

//#define SINGLE_INSTANCE //just one session at a time across all I/O node 
//#define SINGLE_SESSION_OBJECT //just one session per I/O node at a time

//linked list used in write op.
typedef struct _memory_node{
   char* buffer;
   struct _memory_node *next;
   struct _memory_node *previous;
} memory_node;

typedef struct _object_state{
#ifdef SINGLE_SESSION_OBJECT
	struct mutex object_busy;
#endif
	struct mutex operation_synchronizer;
	int valid_bytes;
	char * stream_content;//the I/O node is a buffer in memory
   
   bool priority; //priority level (high or low) for the operations
   bool blocking; //blocking vs non-blocking read and write operations
   unsigned long timeout; //setup of a timeout regulating the awake of blocking operations

        memory_node * head; //head to al written buffer 

} object_state;

typedef struct _packed_work{
        struct work_struct the_work;
        struct file *filp;
        const char *buff;
        size_t len;
        loff_t *off;
        object_state *the_object;
} packed_work;

int write(object_state *the_object, const char *buff, loff_t *off, size_t len){

   //need to lock in any case
   mutex_lock(&(the_object->operation_synchronizer));
         
   if(*off >= OBJECT_MAX_SIZE) {//offset too large
      mutex_unlock(&(the_object->operation_synchronizer));
      return -ENOSPC;//no space left on device
   } 
   
   if(*off > the_object->valid_bytes) {//offset bwyond the current stream size
      mutex_unlock(&(the_object->operation_synchronizer));
      return -ENOSR;//out of stream resources
   }

        memory_node * new_node = kmalloc(sizeof(memory_node), GFP_KERNEL);
        printk("%s: ALLOCATED a new memory node\n",MODNAME);
        if(new_node == NULL){
                printk("%s: unable to allocate a new memory node\n",MODNAME);
                return -1;
        }

        char * new_buffer = kmalloc(len, GFP_KERNEL);
        printk("%s: ALLOCATED %ld bytes\n",MODNAME, len);
        if(new_buffer == NULL){
                printk("%s: unable to allocate memory\n",MODNAME);
                return -1;
        }
        
        memory_node * current_node = the_object->head;
        while(current_node->next != NULL){
                current_node = current_node->next;
        }

        current_node->buffer = new_buffer;
        //needed to link the last node with the new one
        current_node->next = new_node;
        new_node->next = NULL;

        //needed for clear the flow after read op.
        new_node->previous = current_node;

        //returns the number of bytes NOT copied                        
        int ret = copy_from_user(current_node->buffer, buff, len);          

   *off += (len - ret);
   the_object->valid_bytes = *off;
   mutex_unlock(&(the_object->operation_synchronizer));

   return len - ret;
   
}

void delayed_write(unsigned long data){

        size_t len = container_of((void*)data,packed_work,the_work)->len;
        loff_t *off = container_of((void*)data,packed_work,the_work)->off;
        object_state *the_object = container_of((void*)data,packed_work,the_work)->the_object;
        const char *buff = container_of((void*)data,packed_work,the_work)->buff;

        printk("%s: this print comes from kworker daemon with PID=%d - running on CPU-core %d\n",MODNAME,current->pid,smp_processor_id());
    
        write(the_object, buff, off, len);

        printk("%s: releasing the task buffer at address %p - container of task is at %p\n",MODNAME,(void*)data,container_of((void*)data,packed_work,the_work));

        kfree((void*)container_of((void*)data,packed_work,the_work));

        module_put(THIS_MODULE);

}

long put_work(struct file *filp, const char *buff, size_t len, loff_t *off, object_state *the_object){

        packed_work *the_task;

        if(!try_module_get(THIS_MODULE)) return -ENODEV;

        printk("%s: requested deferred work\n",MODNAME);
        
        the_task = kzalloc(sizeof(packed_work),GFP_ATOMIC);//non blocking memory allocation

        if (the_task == NULL) {
                printk("%s: tasklet buffer allocation failure\n",MODNAME);
                module_put(THIS_MODULE);
                return -1;
        }

        the_task->filp = filp;
        the_task->buff = buff;
        the_task->len = len;
        the_task->off = off;
        the_task->the_object = the_object;

        printk("%s: work buffer allocation success - address is %p\n",MODNAME,the_task);

        __INIT_WORK(&(the_task->the_work),(void*)delayed_write,(unsigned long)(&(the_task->the_work)));

        schedule_work(&the_task->the_work);

        return 0;
}
#include "timer.h"
#include "work_queue.h"
#include "info.h"

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static long dev_ioctl(struct file *, unsigned int, unsigned long);

static int Major;            /* Major number assigned to broadcast device driver */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
#define get_major(session)	MAJOR(session->f_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_inode->i_rdev)
#else
#define get_major(session)	MAJOR(session->f_dentry->d_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_dentry->d_inode->i_rdev)
#endif

#define LOW_PRIORITY 0
#define HIGH_PRIORITY 1
#define BLOCKING 0
#define NON_BLOCKING 1 

#ifdef SINGLE_INSTANCE
static DEFINE_MUTEX(device_state);
#endif

object_state objects[MINORS];

static int dev_open(struct inode *inode, struct file *file) {

   int minor;
   minor = get_minor(file);

   if(minor >= MINORS){
	return -ENODEV;
   }
#ifdef SINGLE_INSTANCE
// this device file is single instance
   if (!mutex_trylock(&device_state)) {
		return -EBUSY;
   }
#endif

#ifdef SINGLE_SESSION_OBJECT
   if (!mutex_trylock(&(objects[minor].object_busy))) {
		goto open_failure;
   }
#endif

   printk("%s: device file successfully opened for object with minor %d\n",MODNAME,minor);
//device opened by a default nop
   return 0;


#ifdef SINGLE_SESSION_OBJECT
open_failure:
#ifdef SINGE_INSTANCE
   mutex_unlock(&device_state);
#endif
   return -EBUSY;
#endif

}


static int dev_release(struct inode *inode, struct file *file) {

  int minor;
  minor = get_minor(file);

#ifdef SINGLE_SESSION_OBJECT
   mutex_unlock(&(objects[minor].object_busy));
#endif

#ifdef SINGLE_INSTANCE
   mutex_unlock(&device_state);
#endif

   printk("%s: device file closed\n",MODNAME);
//device closed by default nop
   return 0;

}

/*
the high priority data flow must offer synchronous write operations while the low priority data flow must offer 
an asynchronous execution (based on delayed work) of write operations, while still keeping 
the interface able to synchronously notify the outcome (*).
*/

static ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *off) {

   //WARNING: circle buffer is buonded
   //it does not print the whole buff 
   //printk("%s: char: %c\nchar: %c\nlen: %ld\n",MODNAME, buff[4095], buff[4093], len);
  
  int minor = get_minor(filp);
  int ret;
  object_state *the_object;

  the_object = objects + minor;

   //blocking operation
   if(!the_object->blocking){

      //bring the thread's TCB in waitqueue using sleep/wait service
      blocking(the_object->timeout);
      ret = write(the_object, buff, off, len);
      printk("%s: somebody called a BLOCKING write on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
      
   //non-blocking operation
   }else{
      
      //low priority data flow must offer an asynchronous execution 
      //(based on delayed work) of write operations
         
      if(!the_object->priority){

         //add work to queue
         ret = put_work(filp, buff, len, off, the_object);
         if (ret != 0){

            printk("%s: Error on LOW priority write on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
            return -1;
         }
         
         ret = len - ret; //(*)
         printk("%s: somebody called a NON-BLOCKING LOW priority write on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
          
      //high priority data flow must offer synchronous write operations
      }else{

         ret = write(the_object, buff, off, len);
         printk("%s: somebody called a NON-BLOCKING HIGH priority write on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
            
      }
   }
  
  return ret; //(*)

}

//After read operations, the read data disappear from the flow
static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off) {

  int minor = get_minor(filp);
  int ret;
  object_state *the_object;

  the_object = objects + minor;

   if (!the_object->blocking){

      printk("%s: somebody called a BLOCKING read on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
      //bring the thread's TCB in waitqueue using sleep/wait service
      //blocking(the_object->timeout);

   }else{

      printk("%s: somebody called a NON-BLOCKING read on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
      
   }
   
   //need to lock in any case
   mutex_lock(&(the_object->operation_synchronizer));
   if(*off > the_object->valid_bytes) {
      mutex_unlock(&(the_object->operation_synchronizer));
      return 0;
   }

   //if len of destination buffer is greater then available memory 
   //cut the extra bytes
   //if((the_object->valid_bytes - *off) < len){
    
     // len = the_object->valid_bytes - *off;
   
   //}
   
   //ret = copy_to_user(buff, &(the_object->stream_content[*off]), len);

   //if there is not data to read
   if ((the_object->head)->buffer == NULL){
      mutex_unlock(&(the_object->operation_synchronizer));
      return 0;
   }
   
   int index = 0;
   int lenght = 0;
   int residual_bytes = len;
   int offset = *off;
   memory_node * current_node = the_object->head;

   //PHASE 1: READING
   while(residual_bytes != 0){

      if (current_node->buffer == NULL){
         
         printk("-----------HERE1-----------\n");
         //ret = len
         break;
         

      }else lenght = strlen(current_node->buffer);         
         
         //if bytes read are greater then the requested byte 
         if (lenght >= residual_bytes){
            
            ret = copy_to_user(&buff[index], &current_node->buffer[offset], residual_bytes);
            current_node = current_node->next;
            break;

         }else{

            residual_bytes -= lenght;
            ret = copy_to_user(&buff[index], &current_node->buffer[offset], lenght);
            index += lenght;
            if(current_node->next == NULL){

               break;

            }else current_node = current_node->next;
         }   
   }

   //PHASE 2: REMOVING 
   current_node = the_object->head;
   lenght = strlen(current_node->buffer);
   if(len > lenght){

      if(current_node->next == NULL){

         kfree(current_node->buffer);
         current_node->buffer = NULL;
         //ret = len;
         mutex_unlock(&(the_object->operation_synchronizer));
         return len - ret;

      }else{

         int residual_bytes = len;
         memory_node * last_node = kmalloc(sizeof(memory_node), GFP_KERNEL);
         printk("%s: ALLOCATED a new memory node\n",MODNAME);
         if(last_node == NULL){
               printk("%s: unable to allocate a new memory node\n",MODNAME);
               return -1;
         }

         while(residual_bytes >= lenght){

            residual_bytes -= lenght;
            printk("%s: removing data '%s' from the flow on dev with [major,minor] number [%d,%d]\n",MODNAME,current_node->buffer,get_major(filp),get_minor(filp));
         
            kfree(current_node->buffer);
            last_node = current_node->next;
            
            kfree(current_node);

            if(last_node == NULL){
               
               kfree(last_node->buffer);
               last_node->buffer == NULL;               

            }

            if(last_node->buffer == NULL){
               
               the_object->head = last_node;
               //kfree(last_node);
               mutex_unlock(&(the_object->operation_synchronizer));
               return len - ret;
            
            }else lenght = strlen(last_node->buffer);

            current_node = last_node;

         }

         char *remaning_buff = kmalloc(lenght - residual_bytes, GFP_KERNEL);
         printk("%s: ALLOCATED %d bytes\n",MODNAME, lenght - residual_bytes);
         if(remaning_buff == NULL){
            printk("%s: unable to allocate memory\n",MODNAME);
            return -1;
         }

         strncpy(remaning_buff, &last_node->buffer[residual_bytes], lenght);
           
         kfree(last_node->buffer); 

         last_node->buffer = kmalloc(lenght - residual_bytes, GFP_KERNEL);
         printk("%s: ALLOCATED %d bytes\n",MODNAME, lenght - residual_bytes);
         if(last_node->buffer == NULL){
            printk("%s: unable to allocate memory\n",MODNAME);
            return -1;
         }

         strncpy(last_node->buffer, remaning_buff, lenght - residual_bytes);

         printk("%s: adding data '%s' from the flow on dev with [major,minor] number [%d,%d]\n",MODNAME,last_node->buffer,get_major(filp),get_minor(filp));
            
         kfree(remaning_buff);

         the_object->head = last_node;

         //kfree(last_node);
      
      }

   }else{

      residual_bytes = lenght - len;

      if(residual_bytes == 0){
         kfree(current_node->buffer);
         current_node->buffer = NULL;
         
         if(current_node->next != NULL){
            the_object->head = current_node->next;
            kfree(current_node);
         }
         //ret = len;
         mutex_unlock(&(the_object->operation_synchronizer));
         return len - ret;

      }

      char *remaning_buff = kmalloc(residual_bytes, GFP_KERNEL);
      printk("%s: ALLOCATED %ld bytes\n",MODNAME, residual_bytes);
      if(remaning_buff == NULL){
         printk("%s: unable to allocate memory\n",MODNAME);
         return -1;
      }

      strncpy(remaning_buff, &current_node->buffer[len], lenght);
      
      printk("%s: adding data '%s' from the flow on dev with [major,minor] number [%d,%d]\n",MODNAME,remaning_buff,get_major(filp),get_minor(filp));
         
      kfree(current_node->buffer); 

      current_node->buffer = kmalloc(residual_bytes, GFP_KERNEL);
      printk("%s: ALLOCATED %ld bytes\n",MODNAME, residual_bytes);
      if(current_node->buffer == NULL){
         printk("%s: unable to allocate memory\n",MODNAME);
         return -1;
      }

      strncpy(current_node->buffer, remaning_buff, residual_bytes);
      kfree(remaning_buff);

   }

   //*off += (len - ret);
   
  mutex_unlock(&(the_object->operation_synchronizer));

  return len - ret;

}

/*
The device driver should implement the support for the ioctl(..) service in order to manage the I/O session
as follows:
- setup of the priority level (high or low) for the operations
- blocking vs non-blocking read and write operations
- setup of a timeout regulating the awake of blocking operations
*/

static long dev_ioctl(struct file *filp, unsigned int command, unsigned long param) {

  int minor = get_minor(filp);
  object_state *the_object;

  the_object = objects + minor;
         
  //do here whathever you would like to control the state of the device
   switch(command){

      //2 cannot be used
      case 3:
         the_object->priority = LOW_PRIORITY;
         printk("%s: somebody has set priority level to LOW on dev with [major,minor] number [%d,%d] and command %u \n",MODNAME,get_major(filp),get_minor(filp),command);
         break;
      case 4:
         the_object->priority = HIGH_PRIORITY;
         printk("%s: somebody has set priority level to HIGH on dev with [major,minor] number [%d,%d] and command %u \n",MODNAME,get_major(filp),get_minor(filp),command);
         break;
      case 5:
         the_object->blocking = BLOCKING;
         printk("%s: somebody has set BLOCKING r/w op on dev with [major,minor] number [%d,%d] and command %u \n",MODNAME,get_major(filp),get_minor(filp),command);
         break;
      case 6:
         the_object->blocking = NON_BLOCKING;
         printk("%s: somebody has set NON-BLOCKING r/w on dev with [major,minor] number [%d,%d] and command %u \n",MODNAME,get_major(filp),get_minor(filp),command);
         break;
      case 7:
         the_object->timeout = param;
         printk("%s: somebody has set TIMEOUT on dev with [major,minor] number [%d,%d] and command %u \n",MODNAME,get_major(filp),get_minor(filp),command);
         break;

      default:
         printk("%s: somebody called an invalid setting on dev with [major,minor] number [%d,%d] and command %u \n",MODNAME,get_major(filp),get_minor(filp),command); 
   }
  return 0;

}

static struct file_operations fops = {
  .owner = THIS_MODULE,//do not forget this
  .write = dev_write,
  .read = dev_read,
  .open =  dev_open,
  .release = dev_release,
  .unlocked_ioctl = dev_ioctl
};

int init_module(void) {

	int i;

	//initialize the drive internal state
	for(i=0;i<MINORS;i++){
#ifdef SINGLE_SESSION_OBJECT
		mutex_init(&(objects[i].object_busy));
#endif
		mutex_init(&(objects[i].operation_synchronizer));
		objects[i].valid_bytes = 0;
		objects[i].stream_content = NULL;
		objects[i].stream_content = (char*)__get_free_page(GFP_KERNEL);
      
      //reserve memory for write op.
      objects[i].head = kmalloc(sizeof(memory_node), GFP_KERNEL);
      if(objects[i].head == NULL){
         printk("%s: unable to allocate a new memory node\n",MODNAME);
         return -1;
      }
      
      objects[i].head->next = NULL;
      objects[i].head->buffer = NULL;

		if(objects[i].stream_content == NULL) goto revert_allocation;

	}

	Major = __register_chrdev(0, 0, 256, DEVICE_NAME, &fops);
	//actually allowed minors are directly controlled within this driver

	if (Major < 0) {
	  printk("%s: registering device failed\n",MODNAME);
	  return Major;
	}

	printk(KERN_INFO "%s: new device registered, it is assigned major number %d\n",MODNAME, Major);

	return 0;

revert_allocation:
	for(;i>=0;i--){
		free_page((unsigned long)objects[i].stream_content);
	}
	return -ENOMEM;
}

void cleanup_module(void) {

	int i;
	for(i=0;i<MINORS;i++){
		free_page((unsigned long)objects[i].stream_content);
      kfree(objects[i].head->buffer);
      kfree(objects[i].head);
	}

	unregister_chrdev(Major, DEVICE_NAME);

	printk(KERN_INFO "%s: new device unregistered, it was assigned major number %d\n",MODNAME, Major);

	return;

}

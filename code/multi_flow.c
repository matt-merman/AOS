
/*  
 *  baseline char device driver with limitation on minor numbers - configurable in terms of concurrency 
 */

#define EXPORT_SYMTAB
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>	
#include <linux/tty.h>		/* For the tty declarations */
#include <linux/version.h>	/* For LINUX_VERSION_CODE */

#include "timer.h"
#include "work_queue.h"
#include "info.h"

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

//#define SINGLE_INSTANCE //just one session at a time across all I/O node 
#define SINGLE_SESSION_OBJECT //just one session per I/O node at a time


static int Major;            /* Major number assigned to broadcast device driver */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
#define get_major(session)	MAJOR(session->f_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_inode->i_rdev)
#else
#define get_major(session)	MAJOR(session->f_dentry->d_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_dentry->d_inode->i_rdev)
#endif

#ifdef SINGLE_INSTANCE
static DEFINE_MUTEX(device_state);
#endif

#define LOW_PRIORITY 0
#define HIGH_PRIORITY 1
#define BLOCKING 0
#define NON_BLOCKING 1 

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


} object_state;

#define MINORS 8
object_state objects[MINORS];

#define OBJECT_MAX_SIZE  (4096) //just one page

/* the actual driver */

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
the interface able to synchronously notify the outcome.
*/

static ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *off) {

  int minor = get_minor(filp);
  int ret;
  object_state *the_object;

  the_object = objects + minor;

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

   //low priority data flow must offer an asynchronous execution 
   //(based on delayed work) of write operations
   if(!the_object->priority){

      //add work to queue
      put_work(0);
      //to remove
      if((OBJECT_MAX_SIZE - *off) < len) len = OBJECT_MAX_SIZE - *off;
      ret = copy_from_user(&(the_object->stream_content[*off]),buff,len);

      if (the_object->blocking){

         printk("%s: somebody called a BLOCKING LOW priority write on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));

      }else{

         printk("%s: somebody called a NON-BOLOCKING LOW priority write on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
         
      }

   //high priority data flow must offer synchronous write operations
   }else{

      if((OBJECT_MAX_SIZE - *off) < len) len = OBJECT_MAX_SIZE - *off;
      ret = copy_from_user(&(the_object->stream_content[*off]),buff,len);

      if (the_object->blocking){

         printk("%s: somebody called a BLOCKING HIGH priority write on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));

      }else{

         printk("%s: somebody called a NON-BOLOCKING HIGH priority write on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
         
      }
   }
  
  *off += (len - ret);
  the_object->valid_bytes = *off;
  mutex_unlock(&(the_object->operation_synchronizer));

  return len - ret;

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
      blocking(the_object->timeout);

   }else{

      printk("%s: somebody called a NON-BLOCKING read on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
      
   }
   
   //need to lock in any case
   mutex_lock(&(the_object->operation_synchronizer));
   if(*off > the_object->valid_bytes) {
      mutex_unlock(&(the_object->operation_synchronizer));
      return 0;
   }

   if((the_object->valid_bytes - *off) < len) len = the_object->valid_bytes - *off;
   ret = copy_to_user(buff,&(the_object->stream_content[*off]),len);

   //remove data from the flow
   //...

  *off += (len - ret);
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
	}

	unregister_chrdev(Major, DEVICE_NAME);

	printk(KERN_INFO "%s: new device unregistered, it was assigned major number %d\n",MODNAME, Major);

	return;

}

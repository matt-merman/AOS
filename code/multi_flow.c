#include "info.h"
#include "read.h"
#include "write.h"

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static long dev_ioctl(struct file *, unsigned int, unsigned long);

static int Major; /* Major number assigned to broadcast device driver */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
#define get_major(session) MAJOR(session->f_inode->i_rdev)
#define get_minor(session) MINOR(session->f_inode->i_rdev)
#else
#define get_major(session) MAJOR(session->f_dentry->d_inode->i_rdev)
#define get_minor(session) MINOR(session->f_dentry->d_inode->i_rdev)
#endif

#ifdef SINGLE_INSTANCE
static DEFINE_MUTEX(device_state);
#endif

object_state objects[MINORS][NUM_FLOW];

static int dev_open(struct inode *inode, struct file *file)
{

   session *session;
   int minor = get_minor(file);

   if (minor >= MINORS)
   {
      return -ENODEV;
   }
#ifdef SINGLE_INSTANCE
   // this device file is single instance
   if (!mutex_trylock(&device_state))
   {
      return -EBUSY;
   }
#endif

#ifdef SINGLE_SESSION_OBJECT
   if (!mutex_trylock(&(objects[minor].object_busy)))
   {
      goto open_failure;
   }
#endif

   session = kmalloc(sizeof(session), GFP_KERNEL);
   AUDIT printk("%s: ALLOCATED new session\n", MODNAME);
   if (session == NULL)
   {
      printk("%s: unable to allocate new session\n", MODNAME);
      return -1;
   }

   session->priority = HIGH_PRIORITY;
   session->blocking = NON_BLOCKING;
   session->timeout = 0;
   file->private_data = session;

   AUDIT printk("%s: device file successfully opened for object with minor %d\n", MODNAME, minor);
   // device opened by a default nop
   return 0;

#ifdef SINGLE_SESSION_OBJECT
open_failure:
#ifdef SINGE_INSTANCE
   mutex_unlock(&device_state);
#endif
   return -EBUSY;
#endif
}

static int dev_release(struct inode *inode, struct file *file)
{

   session *session = file->private_data;
   int minor = get_minor(file);

#ifdef SINGLE_SESSION_OBJECT
   mutex_unlock(&(objects[minor].object_busy));
#endif

#ifdef SINGLE_INSTANCE
   mutex_unlock(&device_state);
#endif

   kfree(session);

   AUDIT printk("%s: device file closed\n", MODNAME);
   // device closed by default nop
   return 0;
}

/*
the high priority data flow must offer synchronous write operations while the low priority data flow must offer
an asynchronous execution (based on delayed work) of write operations, while still keeping
the interface able to synchronously notify the outcome (*).
*/

static ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
   object_state *priority_obj;
   int ret;
   int minor = get_minor(filp);
   object_state *the_object = objects[minor];
   session *session = filp->private_data;

   if (session->priority == HIGH_PRIORITY)
   {

      priority_obj = &the_object[HIGH_PRIORITY];

      AUDIT if (session->blocking == BLOCKING) 
         printk("%s: somebody called a BLOCKING HIGH-PRIORITY write on dev with [major,minor] number [%d,%d]\n", MODNAME, get_major(filp), get_minor(filp));
      else 
         printk("%s: somebody called a NON-BLOCKING HIGH-PRIORITY write on dev with [major,minor] number [%d,%d]\n", MODNAME, get_major(filp), get_minor(filp));

      ret = write(priority_obj, buff, off, len, session);
   }
   else
   {

      priority_obj = &the_object[LOW_PRIORITY];

      AUDIT if (session->blocking == BLOCKING) 
         printk("%s: somebody called a BLOCKING LOW-PRIORITY write on dev with [major,minor] number [%d,%d]\n", MODNAME, get_major(filp), get_minor(filp));
      else 
         printk("%s: somebody called a NON-BLOCKING LOW-PRIORITY write on dev with [major,minor] number [%d,%d]\n", MODNAME, get_major(filp), get_minor(filp));

      ret = put_work(filp, buff, len, off, priority_obj, session);
      if (ret != 0)
      {
         printk("%s: Error on LOW-PRIORITY write on dev with [major,minor] number [%d,%d]\n", MODNAME, get_major(filp), get_minor(filp));
         return -1;
      }
      ret = len;
   }

   return ret; //(*)
}

static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off)
{

   object_state *priority_obj;
   int ret;
   int minor = get_minor(filp);
   object_state *the_object = objects[minor];
   session *session = filp->private_data;

   if (session->priority == HIGH_PRIORITY)
   {

      priority_obj = &the_object[HIGH_PRIORITY];

      AUDIT if (session->blocking == BLOCKING) 
         printk("%s: somebody called a BLOCKING read on HIGH-PRIORITY flow on dev with [major,minor] number [%d,%d]\n", MODNAME, get_major(filp), get_minor(filp));
      else 
         printk("%s: somebody called a NON-BLOCKING read on HIGH-PRIORITY flow on dev with [major,minor] number [%d,%d]\n", MODNAME, get_major(filp), get_minor(filp));
   }
   else
   {

      priority_obj = &the_object[LOW_PRIORITY];

      AUDIT if (session->blocking == BLOCKING) 
         printk("%s: somebody called a BLOCKING read on LOW-PRIORITY flow on dev with [major,minor] number [%d,%d]\n", MODNAME, get_major(filp), get_minor(filp));
      else 
         printk("%s: somebody called a NON-BLOCKING read on LOW-PRIORITY flow on dev with [major,minor] number [%d,%d]\n", MODNAME, get_major(filp), get_minor(filp));
   }

   ret = read(priority_obj, buff, off, len, session);

   return ret;
}

/*
The device driver should implement the support for the ioctl(..) service in order to manage the I/O session
as follows:
- setup of the priority level (high or low) for the operations
- blocking vs non-blocking read and write operations
- setup of a timeout regulating the awake of blocking operations
*/

static long dev_ioctl(struct file *filp, unsigned int command, unsigned long param)
{

   session *session;
   session = filp->private_data;

   switch (command)
   {
   case 3:
      session->priority = LOW_PRIORITY;
      AUDIT printk("%s: somebody has set priority level to LOW on dev with [major,minor] number [%d,%d] and command %u \n", MODNAME, get_major(filp), get_minor(filp), command);
      break;
   case 4:
      session->priority = HIGH_PRIORITY;
      AUDIT printk("%s: somebody has set priority level to HIGH on dev with [major,minor] number [%d,%d] and command %u \n", MODNAME, get_major(filp), get_minor(filp), command);
      break;
   case 5:
      session->blocking = BLOCKING;
      AUDIT printk("%s: somebody has set BLOCKING r/w op on dev with [major,minor] number [%d,%d] and command %u \n", MODNAME, get_major(filp), get_minor(filp), command);
      break;
   case 6:
      session->blocking = NON_BLOCKING;
      AUDIT printk("%s: somebody has set NON-BLOCKING r/w on dev with [major,minor] number [%d,%d] and command %u \n", MODNAME, get_major(filp), get_minor(filp), command);
      break;
   case 7:
      session->timeout = param;
      AUDIT printk("%s: somebody has set TIMEOUT on dev with [major,minor] number [%d,%d] and command %u \n", MODNAME, get_major(filp), get_minor(filp), command);
      break;
   default:
      AUDIT printk("%s: somebody called an invalid setting on dev with [major,minor] number [%d,%d] and command %u \n", MODNAME, get_major(filp), get_minor(filp), command);
   }
   return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE, // do not forget this
    .write = dev_write,
    .read = dev_read,
    .open = dev_open,
    .release = dev_release,
    .unlocked_ioctl = dev_ioctl};

int init_module(void)
{

   int i, j;
   // initialize the drive internal state
   for (i = 0; i < MINORS; i++)
   {
#ifdef SINGLE_SESSION_OBJECT
      mutex_init(&(objects[i].object_busy));
#endif

      for (j = 0; j < NUM_FLOW; j++)
      {

         mutex_init(&(objects[i][j].operation_synchronizer));

         // reserve memory to write op.
         objects[i][j].head = kmalloc(sizeof(memory_node), GFP_KERNEL);
         if (objects[i][j].head == NULL)
         {
            printk("%s: unable to allocate a new memory node\n", MODNAME);
            goto revert_allocation;
         }

         objects[i][j].head->next = NULL;
         objects[i][j].head->buffer = NULL;

         init_waitqueue_head(&objects[i][j].wq);
      }
   }

   Major = __register_chrdev(0, 0, 256, DEVICE_NAME, &fops);
   // actually allowed minors are directly controlled within this driver

   if (Major < 0)
   {
      printk("%s: registering device failed\n", MODNAME);
      return Major;
   }

   AUDIT printk(KERN_INFO "%s: new device registered, it is assigned major number %d\n", MODNAME, Major);

   return 0;

revert_allocation:
   for (; i >= 0; i--)
   {
      for (; j >= 0; j--)
      {
         kfree(objects[i][j].head);
      }
   }
   return -ENOMEM;
}

void cleanup_module(void)
{

   int i, j;

   for (i = 0; i < MINORS; i++)
   {
      for (j = 0; j < NUM_FLOW; j++)
      {
         kfree(objects[i][j].head->buffer);
         kfree(objects[i][j].head);
      }
   }

   unregister_chrdev(Major, DEVICE_NAME);

   AUDIT printk(KERN_INFO "%s: new device unregistered, it was assigned major number %d\n", MODNAME, Major);

   return;
}

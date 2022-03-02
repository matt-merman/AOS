
#define EXPORT_SYMTAB
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>	
#include <linux/pid.h>		/* For pid types */
#include <linux/tty.h>		/* For the tty declarations */
#include <linux/version.h>	/* For LINUX_VERSION_CODE */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Di Battista Mattia");

#define OBJECT_MAX_SIZE  (4096) //just one page

#define MODNAME "MULTI_FLOW"

static int multi_flow_open(struct inode *, struct file *);
static int multi_flow_release(struct inode *, struct file *);
static ssize_t multi_flow_write(struct file *, const char *, size_t, loff_t *);

#define DEVICE_NAME "multi_flow"  /* Device file name in /dev/ - not mandatory  */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
#define get_major(session)      MAJOR(session->f_inode->i_rdev)
#define get_minor(session)      MINOR(session->f_inode->i_rdev)
#else
#define get_major(session)      MAJOR(session->f_dentry->d_inode->i_rdev)
#define get_minor(session)      MINOR(session->f_dentry->d_inode->i_rdev)
#endif


static int Major;            /* Major number assigned to broadcast device driver */


/* the actual driver */

typedef struct _object_state{
#ifdef SINGLE_SESSION_OBJECT
        struct mutex object_busy;
#endif
        struct mutex operation_synchronizer;
        int valid_bytes;
        char * stream_content;//the I/O node is a buffer in memory

} object_state;

#define MINORS 8
object_state objects[MINORS];

static int multi_flow_open(struct inode *inode, struct file *file) {

	printk("%s: multi_flow dev closed\n",MODNAME);
	//device opened by a default nop
  	 return 0;
}


static int multi_flow_release(struct inode *inode, struct file *file) {

	printk("%s: multi_flow dev closed\n",MODNAME);
	//device closed by default nop
   	return 0;

}



static ssize_t multi_flow_write(struct file *filp, const char *buff, size_t len, loff_t *off) {

   	int minor = get_minor(filp);
	int ret;
	object_state *the_object;

	the_object = objects + minor;
	printk("%s: somebody called a write on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));

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
	if((OBJECT_MAX_SIZE - *off) < len) len = OBJECT_MAX_SIZE - *off;
	ret = copy_from_user(&(the_object->stream_content[*off]),buff,len);

	*off += (len - ret);
	the_object->valid_bytes = *off;
	mutex_unlock(&(the_object->operation_synchronizer));

	return len - ret;

}

static ssize_t multi_flow_read(struct file *filp, char *buff, size_t len, loff_t *off) {

	int minor = get_minor(filp);
	int ret;
	object_state *the_object;

	the_object = objects + minor;
	printk("%s: somebody called a read on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));

	//need to lock in any case
	mutex_lock(&(the_object->operation_synchronizer));
	if(*off > the_object->valid_bytes) {
			mutex_unlock(&(the_object->operation_synchronizer));
			return 0;
	}
	if((the_object->valid_bytes - *off) < len) len = the_object->valid_bytes - *off;
	ret = copy_to_user(buff,&(the_object->stream_content[*off]),len);

	*off += (len - ret);
	mutex_unlock(&(the_object->operation_synchronizer));

	return len - ret;
	printk("%s: somebody called a read on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));

	return 0;

}



static struct file_operations fops = {
  .owner = THIS_MODULE,
  .write = multi_flow_write,
  .read = multi_flow_read,
  .open =  multi_flow_open,
  .release = multi_flow_release
};


int init_module(void)
{

	Major = register_chrdev(0, DEVICE_NAME, &fops);

	if (Major < 0) {
	  printk("Registering noiser device failed\n");
	  return Major;
	}

	printk(KERN_INFO "multi_flow device registered, it is assigned major number %d\n", Major);


	return 0;
}

void cleanup_module(void)
{

	unregister_chrdev(Major, DEVICE_NAME);

	printk(KERN_INFO "multi_flow device unregistered, it was assigned major number %d\n", Major);
}

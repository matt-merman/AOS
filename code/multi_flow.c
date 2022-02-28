
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

   printk("%s: somebody called a write on multi_flow dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
   return 0;
   //return print_stream_everywhere(buff, len);

}



static struct file_operations fops = {
  .owner = THIS_MODULE,
  .write = multi_flow_write,
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

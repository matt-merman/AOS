MODULE_LICENSE("GPL");
MODULE_AUTHOR("Francesco Quaglia");
MODULE_DESCRIPTION("basic example usage of kernel work queues");

#define MODNAME "CHAR DEV"

#define DEVICE_NAME "my-new-dev"  /* Device file name in /dev/ - not mandatory  */

#define OBJECT_MAX_SIZE  (4096) //just one page
#define MINORS 1
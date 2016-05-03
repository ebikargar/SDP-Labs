/*
 *  chardev_SDP_1.c: Creates a read-only char device that says how many times
 *  you've read from the dev file
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>	/* for put_user */
#include <linux/cdev.h>

/*  
 *  Prototypes - this would normally go in a .h file
 */
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define DEVICE_NAME "chardev_SDP-1"	/* Dev name as it appears in /proc/devices   */
#define BUF_LEN 80					/* Max length of the message from the device */

/* 
 * Global variables are declared as static, so are global within the file. 
 */

static int Major;		/* Major number assigned to our device driver */
static int Device_Open = 0;	/* Is device open?  
				 										 * Used to prevent multiple access to device */
static char msg[BUF_LEN];		/* The msg the device will give when asked */
static char *msg_Ptr;

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

struct cdev *my_cdev;
dev_t dev_no, dev;

/*
 * This function is called when the module is loaded
 */
int init_module(void) {
unsigned int firstminor;
unsigned int count;
int ret;

	my_cdev = cdev_alloc();	
	my_cdev->ops = &fops;
	my_cdev->owner = THIS_MODULE;
	cdev_init(my_cdev, &fops);
  
	firstminor = 0;
	count = 1;
	ret = alloc_chrdev_region( &dev_no , firstminor, count, DEVICE_NAME); // just one minor number
  
	if (ret < 0) {
	  printk(KERN_ALERT "Registering char device failed with %d\n", Major);
	  return ret;
	}

	Major = MAJOR(dev_no);
	dev = MKDEV(Major,0);
	ret = cdev_add(my_cdev, dev,1);		//  one is the number of device minor numbers
	if(ret < 0 ) {
     printk(KERN_INFO "Unable to allocate cdev");
     return ret;
	}
	printk(KERN_INFO "I was assigned major number %d. \n", Major);
	printk(KERN_INFO "To talk to the driver, create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
	printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
	printk(KERN_INFO "the device file.\n");
	printk(KERN_INFO "Remove the device file and module when done.\n");

	return 0;
}

/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void) {

	cdev_del(my_cdev);
    unregister_chrdev_region(dev, 1);
    printk(KERN_INFO "Device %s, Major %d unregistered\n", DEVICE_NAME, Major);
}

/*
 * Methods
 */

/* 
 * Called when a process tries to open the device file, like
 * "cat /dev/chardev_SDP_1"
 */
static int device_open(struct inode *inode, struct file *file)
{
	static int counter = 0;

	if (Device_Open)
		return -EBUSY;

	Device_Open++;
	sprintf(msg, "I already told you %d times Hello world!\n", counter++);
	msg_Ptr = msg;
	try_module_get(THIS_MODULE);				// increment the use counter

	return 0;
}

/* 
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
	Device_Open--;		/* We're now ready for our next caller */

	/* 
	 * Decrement the usage count, otherwise once you opened the file, you'll
	 * never get get rid of the module. 
	 */
	module_put(THIS_MODULE);		// decrement the use counter

	return 0;
}

/* 
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp,		// see include/linux/fs.h 
							char *buffer,			// buffer to fill with data
							size_t length,			// length of the buffer
							loff_t * offset) {
int bytes_read = 0;			//	Number of bytes actually written to the buffer 

	// If we're at the end of the message, return 0 signifying end of file 
	if (*msg_Ptr == 0)
		return 0;

	// Actually put the data into the buffer 
//	printk(KERN_INFO "length =  %d.\n", length);  4096
	while (length && *msg_Ptr) {

		/* 
		 * The buffer is in the user data segment, not the kernel segment
		 *  We have to use put_user which copies data from the kernel data segment to
		 * the user data segment. 
		 */
		put_user(*(msg_Ptr++), buffer++);

		length--;
		bytes_read++;
	}

	return bytes_read;
}

// Called when a process writes to dev file: echo string > chardev_SDP_1 

static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
	printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
	return -EINVAL;
}


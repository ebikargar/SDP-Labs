/*
 *  chardev_SDP_2.c: Creates a read-only char device that says how many times
 *  you've read from the dev file and reads what has been written
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>        /* for put_user */

/*  
 *  Prototypes - this would normally go in a .h file
 */
int init_module (void);
void cleanup_module (void);
static int device_open (struct inode *, struct file *);
static int device_release (struct inode *, struct file *);
static ssize_t device_read (struct file *, char *, size_t, loff_t *);
static ssize_t device_write (struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "chardev_SDP"     /* Dev name as it appears in /proc/devices   */
#define BUF_LEN 80              				/* Max length of the message from the device */

/* 
 * Global variables are declared as static, so are global within the file. 
 */

static int Major;               /* Major number assigned to our device driver */
static int Device_Open = 0;     /* Is device open?  
                                 * Used to prevent multiple access to device */
static char msg[BUF_LEN], write_msg[BUF_LEN];
static char *msg_Ptr, *write_msg_Ptr;

static struct file_operations fops = {
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .release = device_release
};

/*
 * This function is called when the module is loaded
 */
int
init_module (void)
{
  Major = register_chrdev (0, DEVICE_NAME, &fops);

  if (Major < 0) {
    printk (KERN_ALERT "Registering char device failed with %d\n", Major);
    return Major;
  }

  printk (KERN_INFO "I was assigned major number %d. To talk to\n", Major);
  printk (KERN_INFO "the driver, create a dev file with\n");
  printk (KERN_INFO "mknod /dev/%s c %d 0.\n", DEVICE_NAME, Major);
  printk (KERN_INFO "Try to cat and echo+cat to the device file.\n");
  printk (KERN_INFO "Remove the device file and module when done.\n");

  return SUCCESS;
}

/*
 * This function is called when the module is unloaded
 */
void
cleanup_module (void){

unregister_chrdev (Major, DEVICE_NAME);

}

/*
 * Methods
 */

/* 
 * Called when a process tries to open the device file, like
 * "cat /dev/chardev_SDP_2"
 */
static int
device_open (struct inode *inode, struct file *file)
{
  static int counter = 0;
  if (Device_Open)
    return -EBUSY;
  Device_Open++;
  snprintf (msg, 80, "I already told you %d times Hello world!\n", counter++);
  msg_Ptr = msg;
  write_msg_Ptr = write_msg;
  try_module_get (THIS_MODULE); // increment the use counter
  return SUCCESS;
}


//  Called when a process closes the device file.

static int
device_release (struct inode *inode, struct file *file)
{
  Device_Open--;                /* We're now ready for our next caller */

  /* 
   * Decrement the usage count, otherwise once you opened the file you'll
   * never get get rid of the module. 
   */
  module_put (THIS_MODULE);     // decrement the use counter

  return 0;
}

/* 
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */

static ssize_t
device_read (struct file *filp, char *buffer,   /* The buffer to fill with data */
             size_t length,     								/* The length of the buffer     */
             loff_t * offset)										/* Our offset in the file       */
{                               
  int bytes_read = 0;		  /* Number of bytes actually written to the buffer */

  /* put the oen message  into the buffer */
  while (length && *msg_Ptr) {

    /* The buffer is in the user data segment, not the kernel segment;
     * We have to use put_user which copies data from
     * the kernel data segment to the user data segment. 
     */
    put_user (*(msg_Ptr++), buffer++);

    length--;
    bytes_read++;
  }

  /* Actually put the data into the buffer */
  while (write_msg_Ptr && length && *write_msg_Ptr) {
    put_user (*(write_msg_Ptr++), buffer++);
    length--;
    bytes_read++;
  }

  /* Most read functions return the number of bytes put into the buffer */
  return bytes_read;
}


/*  
 * Called when a process writes to dev file: echo string  > /dev/chardev_SDP_2 
 */

static ssize_t
device_write (struct file *filp, const char *buff, size_t len, loff_t * off)
{
  char new_msg[80] = { 0 };
  if (copy_from_user (new_msg, buff, len))
    return -EFAULT;
  if (len > 60)
    len = 65;		// to avoid overflow new_msg buffer
  snprintf (write_msg, 15 + len, "Last Message: %s\n", new_msg);
  write_msg_Ptr = write_msg;

  return len;
}

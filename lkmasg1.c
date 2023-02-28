/**
 * File:	lkmasg1.c
 * Adapted for Linux 5.15 by: John Aedo
 * Class:	COP4600-SP23
 * Victor Yue, Tahsin Islam, Sakun Chitraacharige
*/

#include <linux/module.h>	  // Core header for modules.
#include <linux/device.h>	  // Supports driver model.
#include <linux/kernel.h>	  // Kernel header for convenient functions.
#include <linux/fs.h>		  // File-system support.
#include <linux/uaccess.h>	  // User access copy function support.
#define DEVICE_NAME "lkmasg1" // Device name.
#define CLASS_NAME "char"	  ///< The device class -- this is a character device driver


MODULE_LICENSE("GPL");						 ///< The license type -- this affects available functionality
MODULE_AUTHOR("John Aedo");					 ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("lkmasg1 Kernel Module"); ///< The description -- see modinfo
MODULE_VERSION("0.1");						 ///< A version number to inform users

/**
 * Important variables that store data and keep track of relevant information.
*/

static int major_number;
static char message[256] = {0};
static short size_of_message;
static struct class *lkmasg1Class = NULL;	///< The device-driver class struct pointer
static struct device *lkmasg1Device = NULL; ///< The device-driver device struct pointer

/**
 * Prototype functions for file operations.
*/

static int open(struct inode *, struct file *);
static int close(struct inode *, struct file *);
static ssize_t read(struct file *, char *, size_t, loff_t *);
static ssize_t write(struct file *, const char *, size_t, loff_t *);

/**
 * File operations structure and the functions it points to.
 */

static struct file_operations fops =
{
	.owner = THIS_MODULE,
	.open = open,
	.release = close,
	.read = read,
	.write = write,
};

/**
 * Initializes module at installation
 */

int init_module(void)
{
	printk(KERN_INFO "lkmasg1: installing module.\n");

	// Allocate a major number for the device.
	major_number = register_chrdev(0, DEVICE_NAME, &fops);

	if (major_number < 0)
	{
		printk(KERN_ALERT "lkmasg1 could not register number.\n");
		return major_number;
	}

	printk(KERN_INFO "lkmasg1: registered correctly with major number %d\n", major_number);

	// Register the device class
	lkmasg1Class = class_create(THIS_MODULE, CLASS_NAME);

	if (IS_ERR(lkmasg1Class))
	{ // Check for error and clean up if there is

		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_ALERT "Failed to register device class\n");
		return PTR_ERR(lkmasg1Class); // Correct way to return an error on a pointer

	}

	printk(KERN_INFO "lkmasg1: device class registered correctly\n");

	// Register the device driver
	lkmasg1Device = device_create(lkmasg1Class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);

	if (IS_ERR(lkmasg1Device))
	{								 // Clean up if there is an error

		class_destroy(lkmasg1Class); // Repeated code but the alternative is goto statements
		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_ALERT "Failed to create the device\n");
		return PTR_ERR(lkmasg1Device);

	}

	printk(KERN_INFO "lkmasg1: device class created correctly\n"); // Made it! device was initialized
	return 0;
}

/*
 * Removes module, sends appropriate message to kernel
*/

void cleanup_module(void)
{

	printk(KERN_INFO "lkmasg1: removing module.\n");
	device_destroy(lkmasg1Class, MKDEV(major_number, 0)); // remove the device
	class_unregister(lkmasg1Class);						  // unregister the device class
	class_destroy(lkmasg1Class);						  // remove the device class
	unregister_chrdev(major_number, DEVICE_NAME);		  // unregister the major number
	printk(KERN_INFO "lkmasg1: Goodbye from the LKM!\n");
	unregister_chrdev(major_number, DEVICE_NAME);

	return;
}

/*
 * Opens device module, sends appropriate message to kernel
*/

static int open(struct inode *inodep, struct file *filep)
{
	printk(KERN_INFO "lkmasg1: device opened.\n");
	return 0;
}

/*
 * Closes device module, sends appropriate message to kernel
*/

static int close(struct inode *inodep, struct file *filep)
{
	printk(KERN_INFO "lkmasg1: device closed.\n");
	return 0;
}


/*
 * Reads from device, displays in userspace, and deletes the read data
 */

static ssize_t read(struct file *filep, char *buffer, size_t len, loff_t *offset){

	printk(KERN_INFO "read stub");

	int error_count = 0;
	char temp[sizeof(message)] = {0}; // temporary array to hold message without read data
	int i;
	
	// If not enough data is available to service a read request, the driver must respond with only the amount available (including 0 bytes).
	if(len > size_of_message)
		len = size_of_message;
    
   	error_count = copy_to_user(buffer, message, len);
 
   	if (error_count == 0) {           // if true then have success
		printk(KERN_INFO "lkmasg1: Sent %zu characters to the user: %s\n", len, buffer);
		
		int j = 0;
		for(i = len; i < size_of_message; i++) {
		    temp[j] = message[i];
			j++;
        }
        
        size_of_message = size_of_message - len; // update the size of the message
        strcpy(message, temp);					 // copy temp to message

        return 0;
   	}
   	
   	else {
		printk(KERN_INFO "lkmasg1: Failed to send %d characters to the user\n", error_count);
		return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
   	}
}

/*
 * Writes to the device
 */

static ssize_t write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{

	printk(KERN_INFO "write stub");
	
	int max_message_size = (int)(sizeof(message) - size_of_message - 1); // -1 for null terminator

	// If not enough buffer is available to store a write request, the driver must store only up to the amount available
   	if (len > max_message_size)
		len = max_message_size;

	strncat(message, buffer, len); 			// appending received string with its length
   	size_of_message = strlen(message); 		// store the length of the stored message
   	printk(KERN_INFO "lkmasg1: Received %zu characters from the user\n", len);

	return len;

}
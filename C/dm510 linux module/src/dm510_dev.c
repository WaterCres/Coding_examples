/* Prototype module for second mandatory DM510 assignment */
#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>	
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/wait.h>
/* #include <asm/uaccess.h> */
#include <linux/uaccess.h>
#include <linux/semaphore.h>
/* #include <asm/system.h> */
#include <asm/switch_to.h>
#include <linux/cdev.h>
#include "dm510_dev.h"

struct dm510_pipe *dm510_devices;

/* file operations struct */
static struct file_operations dm510_fops = {
	.owner   = THIS_MODULE,
	.read    = dm510_read,
	.write   = dm510_write,
	.open    = dm510_open,
	.release = dm510_release,
        .unlocked_ioctl   = dm510_ioctl
};

struct dm510_pipe {
		wait_queue_head_t readq, *writeq;
		char *writeBuffer, *writeEnd, *readBuffer, *readEnd;
		char *rp,**wp; 
		int size;
		int readers, writers;
		struct semaphore sem;
		struct cdev cdev;
};


int major =   MAJOR_NUMBER;
int minor =   MIN_MINOR_NUMBER;
int nr_devs = DEVICE_COUNT;	/* number of devices */
int buffer_size = SIZE_BUFF;

// S_IRUGO means the given parameter is a const and wont be changed.
module_param(major, int, S_IRUGO);
module_param(minor, int, S_IRUGO);
module_param(nr_devs, int, S_IRUGO);
// module_param(buffer_size, int, S_IRUGO);

//TODO go through this step by step
/*
* The big and nasty HOW AND WHAT THE SHIT TO DO:
*
* _START_IMPORTANT_NOTES_
*
* i guess <linux/cdev.h> should be included and the stuff belonging to a header should go there('member to include)
* <linux/ioctl.h> should probably also be included
* 
* everywhere mutex_somthing is called i guess it should be semaphore_something?
* (ch 6page 110 in ldd3)(at least if the includes is supposed to make sense)
* "Using int down_interruptible(struct semaphore *sem); requires some
* extra care, however, if the operation is interrupted, the function returns a nonzero
* value, and the caller does not hold the semaphore. Proper use of down_interruptible
* requires always checking the return value and responding accordingly."
*
* open /scull/main.c and /scull/pipe.c in new tab(s)(the ones from daniel)
*
* _END_IMPORTANT_NOTES_
*
*
* make a struct defining the device
* bounded buffer?
*
* initialize devices in the init module just like the main.c,
* except disregard the varible number of devices stuff and the "clever tricks no one understands"
*
* steal the scull_cleanup_module from main.c, change varible names and call it our own
*
* open should probably look like the one from pipe.c
*
* release must undo whatever open does, again look at pipe.c
*
* use our old friends copy_to_user and copy_from_user in the read write methods
* use read and write from pipe.c (or something very similar at least)
*
* for iotcl get the one from main.c
*
* ...across the system...
* "linux-5.16.7/Documentation/userspace-api/ioctl/ioctl-number.rst"
* ioctl defs goto dm510_dev.h:
* 
* 
*/

/* initialize cdev */
static void setup_cdev(struct dm510_pipe *dev, int index) {
	
	// initalize the dev_t's
	int ret_val, devnum = MKDEV(MAJOR_NUMBER, MIN_MINOR_NUMBER + index);

	cdev_init(&dev->cdev, &dm510_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &dm510_fops;
	ret_val = cdev_add(&dev->cdev, devnum,1);

	// failure
	if (ret_val)
	{
		printk(KERN_NOTICE "Error %d adding dm510-%d", ret_val, index);
	}
	
}

// initialize device buffers
static int make_buffer(int size){

	int i;
	for(i = 0; i < DEVICE_COUNT; i++)
	{
		dm510_devices[i].size = size;
		dm510_devices[i].readBuffer = kmalloc(size,GFP_KERNEL);
		memset(dm510_devices[i].readBuffer, 0 , size);
		// check for success
		if (!dm510_devices[i].readBuffer)
		{
			return -ENOMEM;
		}
	}

	// initialize read/write buffers and read/write pointers
	for(i = 0; i < DEVICE_COUNT; i++)
	{
		dm510_devices[i].writeBuffer = dm510_devices[(i+1)%DEVICE_COUNT].readBuffer;
		dm510_devices[i].writeEnd = dm510_devices[i].writeBuffer + size;
		dm510_devices[i].readEnd = dm510_devices[i].readBuffer + size;
		dm510_devices[i].rp = dm510_devices[i].readBuffer;
		dm510_devices[(i+1)%DEVICE_COUNT].wp = &dm510_devices[i].rp;
		dm510_devices[i].writeq = &dm510_devices[(i+1)%DEVICE_COUNT].readq;
			
	}

	return 0;
}


/* called when module is loaded */
int dm510_init_module( void ) {

	int return_value, i;
	
	// register region of our char devices
	return_value = register_chrdev_region(MAJOR_NUMBER,DEVICE_COUNT, DEVICE_NAME);

	// check for succes
	if (return_value < 0)
	{
		printk(KERN_WARNING "dm510: can't get major %d\n", MAJOR_NUMBER);
		return return_value;
	}
	
	// allocate memory space for the devices
	dm510_devices = kmalloc(DEVICE_COUNT * sizeof(struct dm510_pipe), GFP_KERNEL);
	// check for succes
	if (!dm510_devices)
	{
		// return_value = memory allocation error, clean up and exit
		return_value = -ENOMEM;
		unregister_chrdev_region(MAJOR_NUMBER, DEVICE_COUNT);
		return return_value;
	}
	
	// reset memory region
	memset(dm510_devices, 0, DEVICE_COUNT * sizeof(struct dm510_pipe));

	// initialize devices
	for ( i = 0; i < DEVICE_COUNT; i++)
	{
		init_waitqueue_head(&dm510_devices[i].readq);
		sema_init(&dm510_devices[i].sem,1);
		setup_cdev(&dm510_devices[i],i);
	}
	
	return_value = make_buffer(buffer_size);
	if (return_value != 0)
	{
		return return_value;
	}

	printk(KERN_INFO "DM510: Hello from your device!\n");
	return 0;
}

/* Called when module is unloaded */
void dm510_cleanup_module( void ) {

	/* clean up code belongs here */
	int i;
	// int return_value;

	if (!dm510_devices)
	{
		return; // alles gut
	}
	
	// free the registered devices

	for (i = 0; i < DEVICE_COUNT; i++)
	{
		cdev_del(&dm510_devices[i].cdev);
		kfree(dm510_devices[i].readBuffer);
	}
	// free the list of devices
	kfree(dm510_devices);
	// unregister region
	unregister_chrdev_region(MAJOR_NUMBER, DEVICE_COUNT);

	printk(KERN_INFO "DM510: Module unloaded.\n");
	dm510_devices = NULL; 
}


/* Called when a process tries to open the device file */
static int dm510_open( struct inode *inode, struct file *filp ) {
	
	/* device claiming code belongs here */
	struct dm510_pipe *device; // the device to open

	device = container_of(inode->i_cdev, struct dm510_pipe, cdev);
	// point the filp private data to the device that's opened
	filp->private_data = device;

	

	// try to get the mutex lock
	if (down_interruptible(&device->sem))
	{
		return -EAGAIN;
	}
	// if the buffer is not allocated
	if (!device->readBuffer || !device->writeBuffer)
	{
		return -ENOBUFS;
	}


	// set number of readers and writers
	if(filp->f_mode & FMODE_READ)
	{
		device->readers++;
	}
	if (filp->f_mode & FMODE_WRITE)
	{
		device->writers++;
	}
	
	// unlock and return
	up(&device->sem);
	return nonseekable_open(inode, filp);
	//return 0;
}


/* Called when a process closes the device file. */
static int dm510_release( struct inode *inode, struct file *filp ) {

	/* device release code belongs here */
	// get the device
	struct dm510_pipe *device = filp->private_data;
	
	// try to get the mutex lock
	if (down_interruptible(&device->sem))
	{
		return -EAGAIN;
	}
	// decrement number of readers
	if (filp->f_mode & FMODE_READ)
	{
		device->readers--;
	}
	// decrement number of writers
	if (filp->f_mode & FMODE_WRITE)
	{
		device->writers--;
	}
	// release the lock and return
	up(&device->sem);		
	return 0;
}


/* Called when a process, which already opened the dev file, attempts to read from it. */
static ssize_t dm510_read( struct file *filp,
    char *buf,      /* The buffer to fill with data     */
    size_t count,   /* The max number of bytes to read  */
    loff_t *f_pos )  /* The offset in the file           */
{
	
	// get the device
	struct dm510_pipe *device = filp->private_data;


	// get the mutex
	if (down_interruptible(&device->sem))
	{
		return -ERESTART;
	}
	
	// nothing to read
	while (device->rp == device->readBuffer)
	{
		//release mutex
		up(&device->sem);
		// device opend with nonblocking
		if (filp->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		// wait for data to read
		if (wait_event_interruptible(*device->writeq,(device->rp != device->readBuffer)))
		{
			return -ERESTART;
		}
		// try to get the mutex
		if (down_interruptible(&device->sem))
		{
			return -ERESTART;
		}
	}
	
	// data was found, but how much?
	count = min(count, (size_t)(device->rp - device->readBuffer));

	if (!access_ok(buf,count))
	{
		return -EFAULT;
	}
	// copy data to the buffer and check for error
	if (copy_to_user(buf, (device->rp-count), count))
	{
		up(&device->sem);
		return -EFAULT;
	}

	
	// move the pointer to data
	device->rp -= count;
	// printk(KERN_INFO "%ld\n",(device->rp - device->readBuffer));
	// release the mutex
	up(&device->sem);
	// wake up any potential writers and readers
	
	wake_up_interruptible(device->writeq);
	wake_up_interruptible(&device->readq);

	
	
	
	//return number of bytes read
	return count;
}


/* Called when a process writes to dev file */
static ssize_t dm510_write( struct file *filp,
    const char *buf,/* The buffer to get data from      */
    size_t count,   /* The max number of bytes to write */
    loff_t *f_pos )  /* The offset in the file           */
{


	struct dm510_pipe *device = filp->private_data;
	int remaining = device->writeEnd - *device->wp;

	//printk(KERN_INFO "%d\n", remaining);

	if(down_interruptible(&device->sem))
		return -ERESTART;


	// no space
	while( count >= remaining ){
		
		//release mutex
		up(&device->sem);
		// device opend with nonblocking
		if (filp->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		// wait for data to read
		if (wait_event_interruptible(*device->writeq,((device->writeEnd - *device->wp) > count)))
		{
			return -ERESTART;
		}
		remaining = device->writeEnd - *device->wp;

		// try to get the mutex
		if (down_interruptible(&device->sem))
		{
			return -ERESTART;
		}
	}

	//read from user an free semaphore if possible
	if (copy_from_user(*device->wp, buf, count)) {
		up (&device->sem);
		return -EFAULT;
	}

	//add the bytes written into the buffer to count
	*device->wp += count;
	// printk(KERN_INFO "%ld\n",(device->writeEnd - *device->wp));
	up(&device->sem);

	wake_up_interruptible(device->writeq);
	wake_up_interruptible(&device->readq);


	return count; //return number of bytes written
	
	
}

/* called by system call icotl */ 
long dm510_ioctl( 
    struct file *filp, 
    unsigned int cmd,   /* command passed from the user */
    unsigned long arg ) /* argument of the command */
{
	struct dm510_pipe *device = filp->private_data;
	// returns buffer size
	if (cmd == 0) return device->size;

	//changes the size of the buffer
	if(cmd == 1){
		int i;
		buffer_size = arg;
		if (dm510_devices[0].readBuffer != NULL )
		{
			for (i = 0; i < DEVICE_COUNT; i++)
			{
				kfree(dm510_devices[i].readBuffer);
			}
		}		
		
		make_buffer(buffer_size);
	}
	return -ENOTTY; // error code not a typewriter for ioctl 
}

module_init( dm510_init_module );
module_exit( dm510_cleanup_module );

MODULE_INFO(intree,"Y");
MODULE_AUTHOR( "...Andreas Askholm, Hanno Hagge, Rói Olsen" );
MODULE_LICENSE( "GPL" );

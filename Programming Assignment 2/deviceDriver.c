#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#define BUFFER_SIZE 1024

int opened = 0, closed = 0, buffSize = 0;
static char dev_buff[BUFFER_SIZE];

static int my_open(struct inode*, struct file*); 
static ssize_t my_read(struct file*, char*, size_t, loff_t*);
static ssize_t my_write(struct file*, const char*, size_t, loff_t*);
static int my_release(struct inode*, struct file*);

struct file_operations my_fos = {
	.owner = THIS_MODULE,
	.open = my_open,
	.read = my_read,
	.write = my_write,
	.release = my_release
};

static int simple_char_driver_init(void)
{
	printk(KERN_ALERT "simple_char_device_init called\n");
	register_chrdev(42, "simple_char_device", &my_fos);
	return 0;
}

static int simple_char_driver_exit(void)
{
	printk(KERN_ALERT "simple_char_device_exit called\n");
	unregister_chrdev(42, "simple_char_device");
	return 0;
}

static int my_open(struct inode *inode, struct file *file) {
	printk(KERN_ALERT "simple_char_device opened: %d times\n", ++opened);
	return 0;
}

static ssize_t my_read(struct file *filp, char __user *buff, size_t len, loff_t * off) {
	printk(KERN_ALERT "Reading from simple_char_device\n");
	return copy_to_user(buff, dev_buff, len);
}

static ssize_t my_write(struct file *filp, const char *buff, size_t len, loff_t * off) {
	int i=0;
	printk(KERN_ALERT "Writing to simple_char_device\n");
	copy_from_user(dev_buff + buffSize, buff, len);
	while(buff[i++]!=NULL);
	printk(KERN_ALERT "Wrote %d bytes to device\n", i-1);
	buffSize += i-1;
	return i-1;
}

static int my_release(struct inode *inode, struct file *file) {
	printk(KERN_ALERT "simple_char_device closed: %d times\n", ++closed);
	return 0;
}

module_init(simple_char_driver_init);
module_exit(simple_char_driver_exit);

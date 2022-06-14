#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>		/* kmalloc() */

#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/string.h>
#include <linux/fs.h>		/* everything... */
#include <linux/cdev.h>		/* character device */
#include <linux/mutex.h>
#include <linux/string.h>		/* strcat */
#include <asm/switch_to.h>         /* cli(), *_flags */
#include <asm/uaccess.h>        /* copy_*_user */
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include "keyboardInt.h"
#include "keyboard_mapping.h"



static void write_buf(struct circular_buf* b,unsigned char value);
static ssize_t read_buf(struct circular_buf* b);
static struct task_struct *sleeping_task = (void*)0;
int major = K_MAJOR;
int minor = 0;
struct keyboard_stats_dev *k_dev;
DEFINE_SPINLOCK(buf_lock);
DEFINE_SPINLOCK(mr_lock);
void keyboard_tasklet_bh(unsigned long);
DECLARE_TASKLET(keyboard_tasklet, keyboard_tasklet_bh, 0);

/*declare a buffer 512 bytes*/
static char dbuf[512];










void keyboard_tasklet_bh(unsigned long hits) {
  spin_lock(&mr_lock);
  static char bin;
  static char* tmp;
  static int binary;


  sprintf(&bin, "%d", scancode);
  binary = simple_strtol(&bin, &tmp, 10);
  
  /* save the key press/release stat */
  if (scancode & 0x80) 
    binary -= 128;
  
	
  		
  write_buf(&k_dev->inbuf,keyboard_stats[binary].str[0]);
	printk(KERN_INFO "press char %c \n",keyboard_stats[binary].str[0]);
  spin_unlock(&mr_lock);
  return;
}

static char bin;
  static char* tmp;
  static int binary;
irq_handler_t irq_handler (int irq, void *dev_id, struct pt_regs *regs) {
  scancode = inb (0x60);
  


  sprintf(&bin, "%d", scancode);
  binary = simple_strtol(&bin, &tmp, 10);
  
  /* save the key press/release stat */
  if (scancode & 0x80) 
    binary -= 128;
  else
	return 0;
  
		
  write_buf(&k_dev->inbuf,keyboard_stats[binary].str[0]);
		
  //tasklet_schedule(&keyboard_tasklet);
  return (irq_handler_t) IRQ_HANDLED;
}



/*
 * 'open' system call
 */

int k_dev_open(struct inode *inode, struct file *filp) {
  struct keyboard_stats_dev *dev;		/* our device (cdev), which contains 'cdev' */
  dev = container_of(inode->i_cdev, struct keyboard_stats_dev, cdev);
  filp->private_data = dev;
  return 0;
}

/* 
 * Read from the device (write to userspace), 'read' syscall
 */
ssize_t k_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
  struct keyboard_stats_dev *dev = filp->private_data; 
  int retval;
  if(down_interruptible(&dev->sem))
    return -ERESTARTSYS;
  if (*f_pos != 0) {
    goto out;
  }
	
  
  ssize_t c = read_buf(&dev->inbuf);
	/*sleeping_task = current;
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
	printk(KERN_INFO "Woken up thanks  !!\n");*/
  /* write pd_str to userspace */
  if (copy_to_user(buf, dbuf, c)) {
    retval = -EFAULT;
    goto out;
  }

  *f_pos = strlen(dev->stat_str);
  retval = strlen(dev->stat_str);

 out:
  up(&dev->sem);
  return retval;
}

ssize_t k_dev_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp) {
	int retval = 0;
	printk(KERN_INFO "Wake up man !!\n");
  if(sleeping_task)
	wake_up_process(sleeping_task);
  return retval;
}


/*
 * 'close' system call
 */
int k_dev_release(struct inode *inode, struct file *filp) {
  struct keyboard_stats_dev *dev;		/* our device (cdev), which contains 'cdev' */

  /* get the device */
  dev = container_of(inode->i_cdev, struct keyboard_stats_dev, cdev);

  return 0;
}



struct file_operations k_fops = {
  .owner = THIS_MODULE,
  .open  = k_dev_open,
  .read  = k_dev_read,
  .write = k_dev_write,
  .release = k_dev_release,
};


/* 
 * Device Setup for Keyboard Stats Device
 */
int k_dev_setup(struct keyboard_stats_dev *dev) {
  int err;
  dev_t devno = MKDEV(major, minor);
  
  
  cdev_init(&dev->cdev, &k_fops);
  dev->cdev.ops = &k_fops;
  dev->cdev.owner = THIS_MODULE;

  
  err = cdev_add(&dev->cdev, devno, 1);

  if (err) {
    printk(KERN_WARNING "Error during setting up 'keyboard_stats'\n");
    return err;
  }

 
  dev->stat_str = kmalloc(sizeof(char) * K_STR_SIZE, GFP_KERNEL);
  memset(dev->stat_str, 0, sizeof(char) * K_STR_SIZE);

  return 0;
}



static int __init keybrd_int_register(void) {
  int result = -1;
  dev_t dev  = 0;
  
  
  result = alloc_chrdev_region(&dev, minor, 1, "keyboard_stats");
  if (result < 0) {
    major = MAJOR(dev);
    printk(KERN_WARNING "keyboard_stats: can't get major %d\n", major);
    return result;
  }

  major = MAJOR(dev);

      
  k_dev = kmalloc(1 * sizeof(struct keyboard_stats_dev), GFP_KERNEL);


  
  sema_init(&(k_dev->sem),1);	
  result = k_dev_setup(k_dev); 
  if (result)
    return result;

  printk(KERN_INFO "Inserted Module 'keyboard_stats' [%d]\n", major);

     
  result = request_irq (1, (irq_handler_t) irq_handler, IRQF_SHARED, "keyboard_stats", (void *)(irq_handler));
  if (result)
    printk(KERN_INFO "can't get shared interrupt for keyboard\n");
  
  return result;
}


static void __exit keybrd_int_unregister(void) {
  dev_t devno = MKDEV(major, minor);

  free_irq(1, (void *)(irq_handler)); 

  
    kfree(k_dev->stat_str);
    cdev_del(&(k_dev->cdev));

  
  kfree(k_dev);

  unregister_chrdev_region(devno, 1);
  printk(KERN_INFO "Removed Module 'keyboard_stats' [%d]\n", MAJOR(devno));
}



static void write_buf(struct circular_buf* b,unsigned char value)
{
	spin_lock(&buf_lock);
  	b->buf[b->tail] = value;
	b->tail++;
	b->tail%=4096;
	if(sleeping_task&&value=='q')
		wake_up_process(sleeping_task);

	spin_unlock(&buf_lock);
	
}


static ssize_t read_buf(struct circular_buf* b)
{
	spin_lock(&buf_lock);
	if(b->head==b->tail)
	{
		sleeping_task = current;
		set_current_state(TASK_INTERRUPTIBLE);
		spin_unlock(&buf_lock);
		schedule();
		spin_lock(&buf_lock);
	}
	else
	sleeping_task = (void*)0;
	ssize_t i=0;
	while(b->head!=b->tail && i<512 )
	{
  		dbuf[i] = b->buf[b->head];
		b->head++;
		b->head%=4096;
		i++;
	}
	dbuf[i-1] = '\0';
	
 	spin_unlock(&buf_lock);
	return i;
}



MODULE_LICENSE ("GPL");
MODULE_AUTHOR("Vigith Maurice");
module_init(keybrd_int_register);
module_exit(keybrd_int_unregister);



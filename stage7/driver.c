#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>  
#include <linux/gpio.h>    
#include <linux/interrupt.h>
#include <linux/jiffies.h>
 
uint8_t kernel_buffer;
uint8_t button;
#define mem_size     1024 
#define GPIO_11_IN  (11) //button is connected to this GPIO
 
extern unsigned long volatile jiffies;
unsigned long old_jiffie = 0;
 
unsigned int GPIO_irqNumber; 

int echo_value = 0;
module_param(echo_value, int, S_IRUSR);


static irqreturn_t gpio_irq_handler(int irq,void *dev_id) 
{
 
  local_irq_save(flags);
  echo_value = gpio_get_value(GPIO_11_IN);                            
  kernel_buffer = gpio_get_value(GPIO_11_IN); 
  local_irq_restore(flags);
  pr_info("Interrupt Occurred : GPIO_11_IN : %d ", gpio_get_value(GPIO_11_IN));
  return IRQ_HANDLED;
}
 
dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
 
static int __init etx_driver_init(void);
static void __exit etx_driver_exit(void);
 
 
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, 
                char __user *buf, size_t len,loff_t * off);
static ssize_t etx_write(struct file *filp, 
                const char *buf, size_t len, loff_t * off);
 
static struct file_operations fops =
{
  .owner          = THIS_MODULE,
  .read           = etx_read,
  .write          = etx_write,
  .open           = etx_open,
  .release        = etx_release,
};
 
static int etx_open(struct inode *inode, struct file *file)
{
      pr_info("Device File Opened...!!!\n");
      return 0;
}
 
static int etx_release(struct inode *inode, struct file *file)
{
  pr_info("Device File Closed...!!!\n");
  return 0;
}
 
static ssize_t etx_read(struct file *filp, 
                            char __user *buf, size_t len, loff_t *off)
{
  uint8_t gpio_state = 0;
  gpio_state = gpio_get_value(GPIO_11_IN);
  pr_info("Read function : GPIO_11 = %d \n", gpio_state);
  return 0;
 
}
 
static ssize_t etx_write(struct file *filp, 
                const char __user *buf, size_t len, loff_t *off)
{
  pr_info("You can't write to button\n");
  return 0;
}
 
static int __init etx_driver_init(void)
{
  if((alloc_chrdev_region(&dev, 0, 1, "echo_Dev")) <0){
    pr_err("Cannot allocate major number\n");
    unregister_chrdev_region(dev,1);
    return -1;
  }
  pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
 
  cdev_init(&etx_cdev,&fops);
 
  if((cdev_add(&etx_cdev,dev,1)) < 0){
    pr_err("Cannot add the device to the system\n");
    cdev_del(&etx_cdev);
    return -1;
  }
 
  if((dev_class = class_create(THIS_MODULE,"gpio_class")) == NULL){
    pr_err("Cannot create the struct class\n");
    class_destroy(dev_class);
    return -1;
  }
 
  if((device_create(dev_class,NULL,dev,NULL,"echo_pin")) == NULL){
    pr_err( "Cannot create the Device \n");
    device_destroy(dev_class,dev);
    return -1;
  }
 
  if(gpio_is_valid(GPIO_11_IN) == false){
    pr_err("GPIO %d is not valid\n", GPIO_11_IN);
    gpio_free(GPIO_11_IN);
    return -1;
  }
 
  if(gpio_request(GPIO_11_IN,"GPIO_11_IN") < 0){
    pr_err("ERROR: GPIO %d request\n", GPIO_11_IN);
    gpio_free(GPIO_11_IN);
    return -1;
  }
 
  gpio_direction_input(GPIO_11_IN);
 
  GPIO_irqNumber = gpio_to_irq(GPIO_11_IN);  //Get the IRQ number for our GPIO
  pr_info("GPIO_irqNumber = %d\n", GPIO_irqNumber);
 
  if (request_irq(GPIO_irqNumber,             //IRQ number
                  (void *)gpio_irq_handler,   //IRQ handler
                  IRQF_TRIGGER_RISING |
                  IRQF_TRIGGER_FALLING,        //Handler will be called in raising edge and in falling edge
                  "echo_pin",               //used to identify the device name using this IRQ
                  NULL)) {                    //device id for shared IRQ
    pr_err("echo_pin: cannot register IRQ ");
    gpio_free(GPIO_11_IN);
    return -1;
  }
 
  pr_info("Device Driver Insert...Done!!!\n");
  return 0;
}
 
static void __exit etx_driver_exit(void)
{
  free_irq(GPIO_irqNumber,NULL);
  gpio_free(GPIO_11_IN);
  device_destroy(dev_class,dev);
  class_destroy(dev_class);
  cdev_del(&etx_cdev);
  unregister_chrdev_region(dev, 1);
  pr_info("Device Driver Remove...Done!!\n");
}
 
module_init(etx_driver_init);
module_exit(etx_driver_exit);
 
MODULE_LICENSE("ULTRA SEX");
MODULE_AUTHOR("Danov K");
MODULE_DESCRIPTION("GPIO button driver for hard sex");
MODULE_VERSION("1.0");
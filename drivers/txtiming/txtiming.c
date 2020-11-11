#include <linux/clk.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_dma.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
//#include <dt-bindings/dma/axi-dmac.h>
#include <linux/irq.h>
#include <asm/io.h>
#include <linux/fs.h>		//required for fops
#include <linux/uaccess.h>	//required for 'cpoy_from_user' and 'copy_to_user'
#include <linux/signal.h>	//required for kernel-to-userspace signals
#include <linux/string.h>


#define _OPEN_SYS_ITOA_EXT
#define TXTIMING_BASE	0x43c00000

#define RESET_VAL1	0xFFFFFFFF	// load value 1
#define RESET_VAL0	0x00000000	// load value 0

#define DEVICE_NAME     "txtiming"	// device name
#define SUCCESS         0		// success return value

#define BUF_LEN 	80		// max buffer length
#define CLK_FREQ 	100000000	// AXI CLock: 100 MHz

#define aGPIO0_DATA0     0x43c00000      // Enable signals
#define aGPIO0_DATA1     0x43c00004      // Valid signals

#define aGPIO1_DATA0     0x43c00008      // tx timestamp lsb
#define aGPIO1_DATA1     0x43c0000c      // tx timestamp msb

#define aGPIO2_DATA0     0x43c00010      // tx timestamp lsb
#define aGPIO2_DATA1     0x43c00014      // tx timestamp msb

#define aGPIO3_DATA0     0x43c00018      // tx timestamp lsb
#define aGPIO3_DATA1     0x43c0001c      // tx timestamp msb

#define aGPIO4_DATA0     0x43c00020      // tx timestamp lsb
#define aGPIO4_DATA1     0x43c00024      // tx timestamp msb

#define TIMER_TCR0     0x43c00028      // address of timer 0's counter register
#define TIMER_TCR1     0x43c0002c      // address of timer 1's counter register

#define TIMER_OFFSET_TS0     0x43c00030      // timer offset load address lsb
#define TIMER_OFFSET_TS1     0x43c00034      // timer offset load address msb
#define TIMER_OFFSET_EN      0x43c00038      // timer offset load enable


// Round division
#define ROUND_DIV(a, b)		((a + (b / 2)) / b)

// Convert clock frequency in hertz to period in nano seconds
#define HZ_TO_NS(Hz)		ROUND_DIV(1000000000, Hz)

//unsigned long *pSYSCALL_Virtual;	// base address
static int Device_Open = 0;		// device status

static char msg[BUF_LEN];		// the msg the device will give when asked
static char *msg_Ptr;

unsigned long* paGPIO0_DATA0; // Enable signal
unsigned long* paGPIO0_DATA1; // Valid signal

unsigned long* paGPIO1_DATA0; // tx timestamp lsb
unsigned long* paGPIO1_DATA1; // tx timestamp msb

unsigned long* paGPIO2_DATA0; // tx timestamp lsb
unsigned long* paGPIO2_DATA1; // tx timestamp msb

unsigned long* paGPIO3_DATA0; // tx timestamp lsb
unsigned long* paGPIO3_DATA1; // tx timestamp msb

unsigned long* paGPIO4_DATA0; // tx timestamp lsb
unsigned long* paGPIO4_DATA1; // tx timestamp msb

unsigned long *pTIMER_TCR0;             // pointer to timer 0 counter register
unsigned long *pTIMER_TCR1;             // pointer to timer 1 counter register

unsigned long *pTIMER_OFFSET_TS0;	// pointer to timer offset load address lsb
unsigned long *pTIMER_OFFSET_TS1;	// pointer to timer offset load address msb
unsigned long *pTIMER_OFFSET_EN;	// pointer to timer offset load enable

int majorNum;
int mret = 0;
dev_t devNo;  // Major and Minor device numbers combined into 32 bits
struct class *pClass;  // class_create will set this

// write routine (called when write() is used in user-space)
// add offset to the counter in PL
ssize_t txtiming_write(struct file *flip, const char *buf, size_t count, loff_t *offset)
{
	size_t maxdatalen = BUF_LEN, ncopied;
    	char databuf[maxdatalen];
	int64_t offset_ts = 0;
	uint32_t upper_val;
	uint32_t lower_val;
	uint32_t OFFSET_LOAD_EN = 1;
    	
	// Read the value to offset_ts
	if (count < maxdatalen) 
	{
        	maxdatalen = count;
    	}

    	ncopied = copy_from_user(databuf, buf, maxdatalen);
	if (ncopied != 0)
	{	
    		printk("[DBG][drivers][txtiming][txtiming_write][count: %d][error copy from user]\n",count);
		return -EFAULT;
	}

    	databuf[maxdatalen] = 0;

	sscanf(databuf, "%lld", &offset_ts);

	// insert the offset to the hardware
	// split the timestamp
        lower_val = (uint32_t) offset_ts;
        upper_val = (uint32_t) (offset_ts >> 32);
	iowrite32(lower_val,pTIMER_OFFSET_TS0);
	iowrite32(upper_val,pTIMER_OFFSET_TS1);

	// check the written values
	if((lower_val != ioread32(pTIMER_OFFSET_TS0)) || (upper_val != ioread32(pTIMER_OFFSET_TS1)))
	{
    		printk("[DBG][drivers][txtiming][txtiming_write][num: %lld][error writing offset value to the hardware]\n", offset_ts);
		return -EFAULT;
	}

	// load the new counter value
	iowrite32(OFFSET_LOAD_EN,pTIMER_OFFSET_EN);
    	
	printk("[DBG][drivers][txtiming][txtiming_write][count: %d][num: %lld]\n", count, offset_ts);

    	return count;
}

// read routine (called when read() is used in user-space)
ssize_t txtiming_read(struct file *flip, char *buf, size_t size, loff_t *offset)
{
	uint32_t upper_val_tmp;
	uint32_t upper_val;
	uint32_t lower_val;
	uint64_t timerval;
	ssize_t len;

	// read timer value
	upper_val_tmp = ioread32(pTIMER_TCR1);			// Read the upper velue
	lower_val = ioread32(pTIMER_TCR0);			// Read the lower value

	upper_val = ioread32(pTIMER_TCR1);			// Read the upper velue again
	if(upper_val != upper_val_tmp)				// If the value is different, read lower part again
		lower_val = ioread32(pTIMER_TCR0);		// Read the lower velue again

	timerval = (uint64_t) upper_val << 32 | lower_val;	// Combine 2
	
	mret = snprintf (msg, BUF_LEN, "%llu", timerval);
	
	printk("[DBG][drivers][txtiming][txtiming_read][cycles: %llu][size: %d][ret: %d][offset: %d]",timerval,size,mret,(int)(*offset));

	len = min(mret, (int)size);
	if (len <= 0)
        	return 0;
	
	/* read data from my_data->buffer to user buffer */
	if (copy_to_user(buf, &msg, len))
        	return -EFAULT;
	
    	return size;
}

// open routine (called when a device opens /dev/txtiming)
static int txtiming_open(struct inode *inode, struct file *filp)
{

	printk("[DBG][drivers][timer][txtiming][txtiming_open]");
	if (Device_Open)
	{
		printk("[ERR][drivers][timer][txtiming][txtiming_open][One device is already created]");
		return -EBUSY;					// return with an error
	}
	Device_Open++;						// 'open' device
	msg_Ptr = msg;						
	try_module_get(THIS_MODULE);
	return SUCCESS;
}

// close routine (called whne a device closes /dev/txtiming)
static int txtiming_close(struct inode *inode, struct file *file)
{
	printk("[DBG][drivers][timer][txtiming][txtiming_close]");
	Device_Open--;						// 'close' device	
	mret = 0;
	module_put(THIS_MODULE);
	return 0;
}

// device init and file operations
struct file_operations txtiming_fops = {
	.owner = THIS_MODULE,
	.read = txtiming_read,		// read()
	.write = txtiming_write,	// write()
	.open = txtiming_open,		// open()
	.release = txtiming_close,	// close()
};


// init module      
static int __init mod_init(void)
{
	struct device *pDev;

	printk("[DBG][drivers][txtiming][mod_init][Init txtiming module]");
	
	// Timing IP GPIO AXI signals
        paGPIO0_DATA0 = ioremap_nocache(aGPIO0_DATA0,0x4); // Enable signal
        paGPIO0_DATA1 = ioremap_nocache(aGPIO0_DATA1,0x4); // Valid signal

        paGPIO1_DATA0 = ioremap_nocache(aGPIO1_DATA0,0x4); // timestamp lsb
        paGPIO1_DATA1 = ioremap_nocache(aGPIO1_DATA1,0x4); // timestamp msb

        paGPIO2_DATA0 = ioremap_nocache(aGPIO2_DATA0,0x4); // timestamp lsb
        paGPIO2_DATA1 = ioremap_nocache(aGPIO2_DATA1,0x4); // timestamp msb

        paGPIO3_DATA0 = ioremap_nocache(aGPIO3_DATA0,0x4); // timestamp lsb
        paGPIO3_DATA1 = ioremap_nocache(aGPIO3_DATA1,0x4); // timestamp msb

        paGPIO4_DATA0 = ioremap_nocache(aGPIO4_DATA0,0x4); // timestamp lsb
        paGPIO4_DATA1 = ioremap_nocache(aGPIO4_DATA1,0x4); // timestamp msb

        pTIMER_TCR0 = ioremap_nocache(TIMER_TCR0,0x4); // map timer 0 count register
        pTIMER_TCR1 = ioremap_nocache(TIMER_TCR1,0x4); // map timer 1 count register
        
	pTIMER_OFFSET_TS0 = ioremap_nocache(TIMER_OFFSET_TS0,0x4); // timer offset load address lsb
        pTIMER_OFFSET_TS1 = ioremap_nocache(TIMER_OFFSET_TS1,0x4); // timer offset load address msb
        pTIMER_OFFSET_EN = ioremap_nocache(TIMER_OFFSET_EN,0x4); // timer offset enable


	// Register character device
  	majorNum = register_chrdev(0,DEVICE_NAME, &txtiming_fops);
  	if (majorNum < 0) {
    		printk(KERN_ALERT "Error: Could not register txtiming device: %d\n", majorNum);
    		return majorNum;
  	}
	devNo = MKDEV(majorNum, 0);  // Create a dev_t, 32 bit version of numbers
	
	printk("[DBG][drivers][txtiming][mod_init][majorNum: %d]",majorNum);

  	// Create /sys/class/txtiming in preparation of creating /dev/txtiming
  	pClass = class_create(THIS_MODULE, DEVICE_NAME);
  	if (IS_ERR(pClass)) {
    		printk("Error: Could not create txtiming class");
    		unregister_chrdev_region(devNo, 1);
    		return -1;
  	}

  	// Create /dev/txtiming for this char dev
  	if (IS_ERR(pDev = device_create(pClass, NULL, devNo, NULL, DEVICE_NAME))) {
    		printk(KERN_WARNING "txtiming.ko can't create device /dev/txtiming\n");
    		class_destroy(pClass);
    		unregister_chrdev_region(devNo, 1);
    		return -1;
  	}

	printk("[DBG][drivers][txtiming][mod_init][Dev path is created]");
	printk("[DBG][drivers][txtiming][mod_init][64 bit timer is enabled]");

	return SUCCESS;
} 

// exit module
static void __exit mod_exit(void)  		
{
	// Timing IP GPIO AXI signals
	iounmap(paGPIO0_DATA0);
	iounmap(paGPIO0_DATA1);

	iounmap(paGPIO1_DATA0);
	iounmap(paGPIO1_DATA1);

	iounmap(paGPIO2_DATA0);
	iounmap(paGPIO2_DATA1);

	iounmap(paGPIO3_DATA0);
	iounmap(paGPIO3_DATA1);

	iounmap(paGPIO4_DATA0);
	iounmap(paGPIO4_DATA1);

	iounmap(pTIMER_TCR0);
	iounmap(pTIMER_TCR1);

	iounmap(pTIMER_OFFSET_TS0);
	iounmap(pTIMER_OFFSET_TS1);
	iounmap(pTIMER_OFFSET_EN);

	device_destroy(pClass, devNo);  		// Remove the /dev/txtiming
  	class_destroy(pClass);  			// Remove class /sys/class/txtiming
  	unregister_chrdev(majorNum, DEVICE_NAME);  	// Unregister the device
	
	printk("[DBG][drivers][txtiming][txtiming_exit]");
}

static const struct of_device_id my_match_table[] = {
     {
             .compatible = "txtiming_driver",
     },
};


MODULE_DEVICE_TABLE(of, my_match_table);

module_init(mod_init);
module_exit(mod_exit);

MODULE_AUTHOR ("Seyed Samie Mostafavi");
MODULE_DESCRIPTION("Driver for the Xilinx TX Timing IP core.");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("custom:txtiming");

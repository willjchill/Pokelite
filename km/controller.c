#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/usb.h> //USB subsystem functions
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#define DEVICE_NAME "controller"

// Github consulted:
// https://github.com/torvalds/linux/blob/master/drivers/input/joystick/xpad.c
// https://github.com/paroj/xpad/blob/master/xpad.c

// Controller button mapping list - arbitarily chosen and assign a bit value
#define BTN_UP      0x01   // Binary: 0000 0000 0000 0001 (bit 0)
#define BTN_DOWN    0x02   // Binary: 0000 0000 0000 0010 (bit 1)
#define BTN_LEFT    0x04   // Binary: 0000 0000 0000 0100 (bit 2)
#define BTN_RIGHT   0x08   // Binary: 0000 0000 0000 1000 (bit 3)
#define BTN_A       0x10   // Binary: 0000 0000 0001 0000 (bit 4)
#define BTN_B       0x20   // Binary: 0000 0000 0010 0000 (bit 5)
#define BTN_X       0x40   // Binary: 0000 0000 0100 0000 (bit 6)
#define BTN_Y       0x80   // Binary: 0000 0000 1000 0000 (bit 7)
#define BTN_L       0x100  // Binary: 0000 0001 0000 0000 (bit 8)
#define BTN_R       0x200  // Binary: 0000 0010 0000 0000 (bit 9)
#define BTN_START   0x400  // Binary: 0000 0100 0000 0000 (bit 10)
#define BTN_SELECT  0x800  // Binary: 0000 1000 0000 0000 (bit 11)

struct controller_data {
    u16 buttons;
};

static struct controller_data current_state; // keep tracks of the current button state
static dev_t dev_num; // device number assigned from kernel
static struct cdev c_dev; // link dev_num to the specific functions
static struct class *dev_class; // create /dev/controller
static struct usb_device *controller_udev = NULL; // USB controller ptr; NULL = no controller connected 
static struct urb *controller_urb = NULL; // struct to request data from controller
static unsigned char *usb_buffer; // buffer for USB data
static dma_addr_t usb_buffer_dma; // store the physical address

// Parsing the controller input 
static void parse_controller_data(unsigned char *data)
{
    u16 buttons = 0; //no button pressed - default state

    // Sources consulted:
    // https://www.partsnotincluded.com/understanding-the-xbox-360-wired-controllers-usb-data/?utm_source=chatgpt.com
    // For the purpose what we are trying to do, only data[2] and data[3] matters
    // because in the article going over the details of XBox USB mapping, 
    // Bytes 2 and 3 contain the packed states for the controller’s digital buttons in a bit array, where ‘1’ is pressed and ‘0’ is 
    // released. This includes all 8 surface buttons, the 2 joystick buttons, the center ‘Xbox’ button, and the directional pad.

    // Therefore, parsing the controller data consists of checking the value of either 
    // data[2] or data[3] and the corresponding button value defined above to see whether both
    // are set or not. If that is the case, then the button is pressed and vice-versa.

    // D-pad
    if (data[2] & 0x01) buttons |= BTN_UP;
    if (data[2] & 0x02) buttons |= BTN_DOWN;
    if (data[2] & 0x04) buttons |= BTN_LEFT;
    if (data[2] & 0x08) buttons |= BTN_RIGHT;
    
    // Start/Select
    if (data[2] & 0x10) buttons |= BTN_START;
    if (data[2] & 0x20) buttons |= BTN_SELECT;
    
    // Shoulder
    if (data[3] & 0x01) buttons |= BTN_L;
    if (data[3] & 0x02) buttons |= BTN_R;
    
    // Face buttons
    if (data[3] & 0x10) buttons |= BTN_A;
    if (data[3] & 0x20) buttons |= BTN_B;
    if (data[3] & 0x40) buttons |= BTN_X;
    if (data[3] & 0x80) buttons |= BTN_Y;
    
    current_state.buttons = buttons;
}

// URB callback - runs when controller send input
static void controller_irq(struct urb *urb)
{
    int ret;
    
    if (urb->status == 0) { // if urb status is sucessful (0)
        // No error
        parse_controller_data(usb_buffer);
    }
    ret = usb_submit_urb(urb, GFP_ATOMIC); // submit to receibe the next input from controller
    // Error
    if (ret && ret != -EPERM) {
        printk(KERN_ERR "controller: urb resubmit failed\n");
        // if device still connected but failure to submit
        // if device is unplugged, usb_submit_urb() returns -EPERM.
    }
}

static int device_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t device_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    if (len < sizeof(struct controller_data))
        return -EINVAL;
    
    if (copy_to_user(buf, &current_state, sizeof(struct controller_data)))
        return -EFAULT;
    
    return sizeof(struct controller_data);
}

// define func operation forthe controller
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
};

// When the controller is pluged in using the USB
// Check pid if it is an xbox controller
static int controller_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct usb_endpoint_descriptor *endpoint;
    int pipe, maxp;
    int ret;
    
    printk(KERN_INFO "Controller: USB controller connected\n");
    
    controller_udev = interface_to_usbdev(interface); // get the whole usb device not just an interface 
    endpoint = &interface->cur_altsetting->endpoint[0].desc; //find the port where the controller sends button data - Which USB wire connection sends the button information
    
    // Pipe set up using the endpoint above
    pipe = usb_rcvintpipe(controller_udev, endpoint->bEndpointAddress); // pipe to receive button data
    maxp = usb_maxpacket(controller_udev, pipe, 0); 
    
    // Allocate buffer to store the input data
    usb_buffer = usb_alloc_coherent(controller_udev, 32, GFP_KERNEL, &usb_buffer_dma);
    if (!usb_buffer) {
        printk(KERN_ERR "Controller: Failed to allocate buffer\n");
        return -ENOMEM;
    }
    
    // Allocate URB to notify when the controller sends new input
    controller_urb = usb_alloc_urb(0, GFP_KERNEL);
    if (!controller_urb) {
        printk(KERN_ERR "Controller: Failed to allocate urb\n");
        usb_free_coherent(controller_udev, 32, usb_buffer, usb_buffer_dma);
        return -ENOMEM;
    }
    
    // Setup URB -> put data in it
    usb_fill_int_urb(controller_urb, controller_udev, pipe, usb_buffer, 32,
                     controller_irq, NULL, endpoint->bInterval);
    controller_urb->transfer_dma = usb_buffer_dma;
    controller_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
    
    // Submit URB - start waiting for input data
    ret = usb_submit_urb(controller_urb, GFP_KERNEL);
    if (ret) {
        printk(KERN_ERR "Controller: Failed to submit urb\n");
        usb_free_urb(controller_urb);
        usb_free_coherent(controller_udev, 32, usb_buffer, usb_buffer_dma);
        return ret;
    }
    
    return 0;
}

// USB disconnect -> getting rid of all associated parts when disconnected
static void controller_disconnect(struct usb_interface *interface)
{
    printk(KERN_INFO "Controller: USB controller disconnected\n");
    
    if (controller_urb) {
        usb_kill_urb(controller_urb);
        usb_free_urb(controller_urb);
        controller_urb = NULL;
    }
    
    if (usb_buffer) {
        usb_free_coherent(controller_udev, 32, usb_buffer, usb_buffer_dma);
        usb_buffer = NULL;
    }
    
    controller_udev = NULL;
}
// USB device id - NOTE !!!
// The first number - 0x045e is the Microsoft Vendor ID -> Common for all Microsoft usb products
// The second number - Device ID -> WILL CHANGES BASED ON THE CONTROLLER MODEL - CHECK AND MODIFY WHEN NEEDED
// Check the specific device number using lsusb
// The USB prob func will only run if the controller plugged in is one of the listed ones here, so needs to add 
// when neccessary
static struct usb_device_id controller_table[] = {
    { USB_DEVICE(0x045e, 0x028e) },  // Xbox 360 Controller
    { USB_DEVICE(0x045e, 0x02dd) },  // Xbox One Controller
    { USB_INTERFACE_INFO(USB_CLASS_VENDOR_SPEC, 93, 1) },  // Generic XInput -  Also modified as needed
    { }
};
MODULE_DEVICE_TABLE(usb, controller_table);

// Assigning the funcs for the usb driver
static struct usb_driver controller_driver = {
    .name = "game_controller",
    .probe = controller_probe,
    .disconnect = controller_disconnect,
    .id_table = controller_table,
};

// Module init -> insmod controller.ko 
static int __init controller_init(void)
{
    int ret;
    
    printk(KERN_INFO "Controller: Loading module\n");
    
    // Register character device
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "Controller: failed to allocate chrdev\n");
        return ret;
    }
    
    cdev_init(&c_dev, &fops);
    ret = cdev_add(&c_dev, dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }
    
    dev_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(dev_class)) {
        cdev_del(&c_dev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(dev_class);
    }
    
    device_create(dev_class, NULL, dev_num, NULL, DEVICE_NAME);
    
    // Register USB driver
    ret = usb_register(&controller_driver);
    if (ret < 0) {
        device_destroy(dev_class, dev_num);
        class_destroy(dev_class);
        cdev_del(&c_dev);
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }
    
    printk(KERN_INFO "Controller: Module loaded, device at /dev/%s\n", DEVICE_NAME);
    return 0;
}

// Module exit
static void __exit controller_exit(void)
{
    printk(KERN_INFO "Controller: Unloading module\n");
    
    usb_deregister(&controller_driver);
    device_destroy(dev_class, dev_num);
    class_destroy(dev_class);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev_num, 1);
}

module_init(controller_init);
module_exit(controller_exit);

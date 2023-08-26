#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include <linux/slab.h>
#include <linux/cdev.h>

#include <linux/platform_device.h>
#include <linux/of.h>

#define DEVICE_NAME "simple_device"
#define BUFFER_SIZE 1024

static int simple_device_major;

static char buffer[BUFFER_SIZE];
static int position;

static struct class * simple_device_class;

static int simple_drv_open(struct inode *inode, struct file *file)
{
    position = 0;
    return 0;
}

static ssize_t simple_drv_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    ssize_t bytes_read = 0;

    if (*offset >= BUFFER_SIZE)
        return 0;

    if (*offset + count > BUFFER_SIZE)
        count = BUFFER_SIZE - *offset;

    if (copy_to_user(buf, buffer + *offset, count))
        return -EFAULT;

    *offset += count;
    bytes_read = count;

    return bytes_read;
}

static ssize_t simple_drv_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    ssize_t bytes_written = 0;

    if (*offset >= BUFFER_SIZE)
        return -ENOSPC;

    if (*offset + count > BUFFER_SIZE)
        count = BUFFER_SIZE - *offset;

    if (copy_from_user(buffer + *offset, buf, count))
        return -EFAULT;

    *offset += count;
    bytes_written = count;

    return bytes_written;
}

static struct file_operations fops = {
    .open = simple_drv_open,
    .read = simple_drv_read,
    .write = simple_drv_write,
};

// static int __init simple_drv_init(void)
// {
//     int ret = register_chrdev(0, DEVICE_NAME, &fops);
//     if (ret < 0) {
//         printk(KERN_ALERT "Failed to register character device: %d\n", ret);
//         return ret;
//     }

//     printk(KERN_INFO "Simple driver loaded.\n");
//     return 0;
// }

// static void __exit simple_drv_exit(void)
// {
//     unregister_chrdev(0, DEVICE_NAME);
//     printk(KERN_INFO "Simple driver unloaded.\n");
// }

// module_init(simple_drv_init);
// module_exit(simple_drv_exit);

static int simple_drv_suspend(struct device *dev)
{
	printk("simple_dev: suspend\n");
	return 0;
}

static int simple_drv_resume(struct device *dev)
{
	printk("simple_dev: resume\n");
	return 0;
}

static const struct dev_pm_ops simple_drv_pm_ops = {
	.suspend = simple_drv_suspend,
	.resume  = simple_drv_resume,
};

static int simple_drv_probe(struct platform_device *pdev)
{
    printk(KERN_INFO "Simple driver probe\n");
    int ret;
    dev_t dev;
    int i;
    ret = alloc_chrdev_region(&dev, 0, 1, "simple_device");
    if(ret)
    {
        goto id_err;
    }
    
	simple_device_major = MAJOR(dev);
    printk(KERN_INFO "alloc chrdev %d\n", simple_device_major);

    printk(KERN_INFO "device of node %p\n", pdev->dev.of_node);
    const struct property *prop = of_find_property(pdev->dev.of_node, "test-names", NULL);
    char *p = prop->value;
    printk(KERN_INFO "test-names %s\n", p);
    printk(KERN_INFO "test-names length %d\n", prop->length);
    
    printk(KERN_INFO "node name %s\n", pdev->dev.of_node->name);
    printk(KERN_INFO "node full name %s\n", pdev->dev.of_node->full_name);

    printk(KERN_INFO "node child name %s\n", pdev->dev.of_node->child->name);

    prop = of_find_property(pdev->dev.of_node, "test_value", NULL);
    if (prop == NULL)
    {
        printk(KERN_INFO "prop NULL\n");
    }
    p = prop->value;
    if (p == NULL)
    {
        printk(KERN_INFO "prop->value NULL\n");
    }
    // printk(KERN_INFO "cell_name %s\n", p);
    printk(KERN_INFO "test_value length %d\n", prop->length);

    prop = of_find_property(pdev->dev.of_node->child, "child_param", NULL);
    p = prop->value;
    printk(KERN_INFO "child_param %s\n", p);

    unsigned int value;
    of_property_read_u32_index(pdev->dev.of_node, "test_value", 0, &value);
    printk(KERN_INFO "test_value 0 %d\n", value);

    of_property_read_u32_index(pdev->dev.of_node, "test_value", 1, &value);
    printk(KERN_INFO "test_value 1 %d\n", value);

    of_property_read_u32_index(pdev->dev.of_node, "test_value", 2, &value);
    printk(KERN_INFO "test_value 2 %d\n", value);    

    int ele_count = 0;
    ele_count = of_property_count_elems_of_size(pdev->dev.of_node, "test_value", 4);
    printk(KERN_INFO "test_value ele_count %d\n", ele_count);

    const char *out_string;
    of_property_read_string(pdev->dev.of_node, "test_string", &out_string);
    printk(KERN_INFO "test_string %s\n", out_string); 

    struct cdev *my_dev = cdev_alloc();
    if(my_dev == NULL)
    {
        ret = -ENOMEM;
		goto mem_err;
    }

    cdev_init(my_dev, &fops);
	my_dev->owner = THIS_MODULE;
	ret = cdev_add(my_dev, dev, 1);
	if (ret)
		goto add_err;
    
    platform_set_drvdata(pdev, my_dev);

    struct device * class_dev = device_create(simple_device_class, NULL, dev, NULL, "simple_device");
    if (IS_ERR(class_dev)) {
		ret = PTR_ERR(class_dev);
		goto dev_err;
	}
    
    return 0;
dev_err:
add_err:
    kfree(my_dev);
mem_err:
    unregister_chrdev_region(dev, 1);
id_err:
	return ret;
}

static int simple_drv_remove(struct platform_device *pdev)
{
    dev_t dev;

	dev = MKDEV(simple_device_major, 0);
    struct cdev *cdev = platform_get_drvdata(pdev);

    device_destroy(simple_device_class, dev);
	cdev_del(cdev);
	kfree(cdev);
	unregister_chrdev_region(dev, 1);
	return 0;
}

static const struct of_device_id of_simple_device_id[] = {
  { .compatible = "virtual,simple-device" },
  { },
};

MODULE_DEVICE_TABLE(of, of_simple_device_id);

struct platform_driver simple_drv = {
	.driver = {
		.name    = "simple_dev",
        .of_match_table = of_simple_device_id,
		.owner   = THIS_MODULE,
		.pm      = &simple_drv_pm_ops,
	},
	.probe   = simple_drv_probe,
	.remove  = simple_drv_remove,
};

static int __init simple_drv_init(void)
{
    int ret;
    printk(KERN_INFO "Simple driver loaded.\n");

    simple_device_class = class_create(THIS_MODULE, "simple device class");
    if(IS_ERR(simple_device_class))
        return PTR_ERR(simple_device_class);
	ret = platform_driver_register(&simple_drv);
    if(ret)
        class_destroy(simple_device_class);
	// platform_device_register(&pdev1);

	return 0;
}

static void __exit simple_drv_exit(void)
{
    printk(KERN_INFO "Simple driver unloaded.\n");
	platform_driver_unregister(&simple_drv);
    class_destroy(simple_device_class);
	// platform_device_unregister(&pdev0);
}

module_init(simple_drv_init);
module_exit(simple_drv_exit);

// module_platform_driver(pdrv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple virtual device driver for Raspberry Pi 4B");

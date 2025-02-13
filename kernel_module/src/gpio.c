#include "gpio.h"
#include "gpio_debugfs.h"
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/timer.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sandesh Ghimire");
MODULE_DESCRIPTION("Advanced GPIO LED Controller");
MODULE_VERSION("2.0");

static struct gpio_led_data *led_data[MAX_GPIOS];
static int num_gpios = 0;
static dev_t dev_num;
static struct class *device_class;
static struct cdev gpio_cdev;

// Function prototypes
static int led_open(struct inode *inode, struct file *file);
static int led_close(struct inode *inode, struct file *file);
static ssize_t led_read(struct file *file, char __user *buf, size_t count,
                        loff_t *offset);
static ssize_t led_write(struct file *file, const char __user *buf,
                         size_t count, loff_t *offset);
static long led_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static void blink_timer_callback(struct timer_list *t);
static void thermal_check_work(struct work_struct *work);
static irqreturn_t led_trigger_handler(int irq, void *dev_id);
static int led_suspend(struct device *dev);
static int led_resume(struct device *dev);

// Enhanced file operations
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_close,
    .read = led_read,
    .write = led_write,
    .unlocked_ioctl = led_ioctl,
};

// Open function
static int led_open(struct inode *inode, struct file *file) {
  printk(KERN_INFO "%s: Device opened\n", DEVICE_NAME);
  return 0;
}

// Close function
static int led_close(struct inode *inode, struct file *file) {
  printk(KERN_INFO "%s: Device closed\n", DEVICE_NAME);
  return 0;
}

// Read function (returns current LED state)
static ssize_t led_read(struct file *file, char __user *buf, size_t count,
                        loff_t *offset) {
  char state_str[2];
  sprintf(state_str, "%d", led_state);
  size_t len = strlen(state_str);

  if (*offset >= len)
    return 0; // End of file

  if (count > len - *offset)
    count = len - *offset;

  if (copy_to_user(buf, state_str + *offset, count))
    return -EFAULT;

  *offset += count;
  printk(KERN_INFO "%s: Read %zu bytes, offset = %lld\n", DEVICE_NAME, count,
         *offset);
  return count;
}

// Write function (to control the LED)
static ssize_t led_write(struct file *file, const char __user *buf,
                         size_t count, loff_t *offset) {
  char kbuf[2];

  if (count > 1)
    return -EINVAL;

  if (copy_from_user(kbuf, buf, count))
    return -EFAULT;

  if (kbuf[0] == '1') {
    gpio_set_value(gpio_pin, 1);
    led_state = 1;
    printk(KERN_INFO "%s: LED ON\n", DEVICE_NAME);
  } else if (kbuf[0] == '0') {
    gpio_set_value(gpio_pin, 0);
    led_state = 0;
    printk(KERN_INFO "%s: LED OFF\n", DEVICE_NAME);
  } else {
    printk(KERN_INFO "%s: Invalid command\n", DEVICE_NAME);
    return -EINVAL;
  }

  return count;
}

// Timer callback for LED blinking
static void blink_timer_callback(struct timer_list *t) {
  struct gpio_led_data *led = from_timer(led, t, blink_timer);

  led->state = !led->state;
  gpio_set_value(led->gpio_pin, led->state);

  if (led->blinking) {
    unsigned long delay =
        led->state ? led->blink_delay_on : led->blink_delay_off;
    mod_timer(&led->blink_timer, jiffies + msecs_to_jiffies(delay));
  }
}

// IOCTL handler
static long led_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
  struct gpio_led_data *led = file->private_data;
  struct led_blink_params blink_params;
  int brightness;

  if (!led)
    return -ENODEV;

  switch (cmd) {
  case LED_SET_BRIGHTNESS:
    if (get_user(brightness, (int __user *)arg))
      return -EFAULT;
    if (brightness < 0 || brightness > 100)
      return -EINVAL;

    led->brightness = brightness;
    gpio_set_value(led->gpio_pin, brightness > 0);
    break;

  case LED_SET_BLINK:
    if (copy_from_user(&blink_params, (struct led_blink_params __user *)arg,
                       sizeof(blink_params)))
      return -EFAULT;

    led->blinking = true;
    led->blink_delay_on = blink_params.delay_on;
    led->blink_delay_off = blink_params.delay_off;
    mod_timer(&led->blink_timer,
              jiffies + msecs_to_jiffies(led->blink_delay_on));
    break;

  case LED_RESET:
    led->blinking = false;
    del_timer(&led->blink_timer);
    gpio_set_value(led->gpio_pin, 0);
    led->state = 0;
    led->brightness = 0;
    break;

  default:
    return -ENOTTY;
  }

  return 0;
}

// Thermal monitoring work function
static void thermal_check_work(struct work_struct *work) {
  struct gpio_led_data *led = container_of(work, struct gpio_led_data, work);
  int temp;

  temp = get_cpu_temp(); // Implementation needed
  led->last_temp = temp;

  if (temp >= led->thermal.temp_threshold && !led->thermal_shutdown) {
    led->thermal_shutdown = true;
    gpio_set_value(led->gpio_pin, 0);
  } else if (temp <= (led->thermal.temp_threshold - led->thermal.hysteresis) &&
             led->thermal_shutdown) {
    led->thermal_shutdown = false;
    gpio_set_value(led->gpio_pin, led->state);
  }

  if (led->thermal.auto_throttle) {
    schedule_delayed_work(&led->work, HZ * 5); // Check every 5 seconds
  }
}

// Interrupt handler for external trigger
static irqreturn_t led_trigger_handler(int irq, void *dev_id) {
  struct gpio_led_data *led = dev_id;
  unsigned long flags;

  spin_lock_irqsave(&led->lock, flags);
  led->state = !led->state;
  led->stats.switches++;
  gpio_set_value(led->gpio_pin, led->state);
  spin_unlock_irqrestore(&led->lock, flags);

  return IRQ_HANDLED;
}

// Power management suspend
static int led_suspend(struct device *dev) {
  struct gpio_led_data *led = dev_get_drvdata(dev);

  if (led->pwm)
    pwm_disable(led->pwm);

  gpio_set_value(led->gpio_pin, 0);
  led->stats.power_cycles++;

  return 0;
}

// Power management resume
static int led_resume(struct device *dev) {
  struct gpio_led_data *led = dev_get_drvdata(dev);

  if (led->pwm && !led->thermal_shutdown)
    pwm_enable(led->pwm);

  return 0;
}

static SIMPLE_DEV_PM_OPS(led_pm_ops, led_suspend, led_resume);

// Add helper function for temperature reading
static int get_cpu_temp(void) {
  struct thermal_zone_device *thermal;
  int temp = 0;

  thermal = thermal_zone_get_zone_by_name("cpu-thermal");
  if (!IS_ERR(thermal)) {
    thermal_zone_get_temp(thermal, &temp);
  }

  return temp / 1000; // Convert to degrees C
}

// Fix gpio_led_probe function
static int gpio_led_probe(struct platform_device *pdev) {
  struct device_node *np = pdev->dev.of_node;
  int i, ret;
  struct gpio_led_data *led;

  if (!np)
    return -ENODEV;

  num_gpios = of_gpio_count(np);
  if (num_gpios > MAX_GPIOS)
    num_gpios = MAX_GPIOS;

  for (i = 0; i < num_gpios; i++) {
    led = devm_kzalloc(&pdev->dev, sizeof(*led), GFP_KERNEL);
    if (!led)
      return -ENOMEM;

    led_data[i] = led;

    // Initialize basic LED data
    led->gpio_pin = of_get_gpio(np, i);
    if (!gpio_is_valid(led->gpio_pin)) {
      dev_err(&pdev->dev, "Invalid GPIO %d\n", i);
      return -EINVAL;
    }

    // Set default values
    led->thermal.temp_threshold = 80; // 80°C default
    led->thermal.hysteresis = 5;
    led->thermal.auto_throttle = true;

    ret = devm_gpio_request_one(&pdev->dev, led->gpio_pin, GPIOF_OUT_INIT_LOW,
                                "led-gpio");
    if (ret) {
      dev_err(&pdev->dev, "Failed to request GPIO %d\n", led->gpio_pin);
      return ret;
    }

    // Initialize timer and work
    timer_setup(&led->blink_timer, blink_timer_callback, 0);
    INIT_DELAYED_WORK(&led->work, thermal_check_work);
    spin_lock_init(&led->lock);

    // Setup PWM if available
    led->pwm = devm_pwm_get(&pdev->dev, kasprintf(GFP_KERNEL, "led%d", i));
    if (!IS_ERR(led->pwm)) {
      led->hardware_pwm = true;
      pwm_enable(led->pwm);
    }

    // Start thermal monitoring if enabled
    if (led->thermal.auto_throttle)
      schedule_delayed_work(&led->work, HZ);

    // Initialize debugfs entries
    led_debugfs_init(led);
  }

  return 0;
}

// Fix cleanup in remove function
static int gpio_led_remove(struct platform_device *pdev) {
  int i;
  struct gpio_led_data *led;

  for (i = 0; i < num_gpios; i++) {
    led = led_data[i];
    if (led) {
      cancel_delayed_work_sync(&led->work);
      del_timer_sync(&led->blink_timer);
      if (led->pwm)
        pwm_disable(led->pwm);
      gpio_set_value(led->gpio_pin, 0);
      led_debugfs_remove(led);
    }
  }

  return 0;
}

// Device tree matching table
static const struct of_device_id gpio_led_of_match[] = {
    {.compatible = "gpio-led-controller"},
    {},
};
MODULE_DEVICE_TABLE(of, gpio_led_of_match);

// Platform driver structure
static struct platform_driver gpio_led_driver = {
    .probe = gpio_led_probe,
    .remove = gpio_led_remove,
    .driver =
        {
            .name = DEVICE_NAME,
            .of_match_table = gpio_led_of_match,
            .pm = &led_pm_ops,
        },
};

// Fix module initialization
static int __init my_module_init(void) {
  int ret;

  ret = platform_driver_register(&gpio_led_driver);
  if (ret)
    return ret;

  // --- GPIO Request and Direction ---
  if (!gpio_is_valid(gpio_pin)) {
    printk(KERN_ERR "%s: Invalid GPIO pin: %d\n", DEVICE_NAME, gpio_pin);
    return -ENODEV;
  }

  ret = gpio_request(gpio_pin, "led-gpio");
  if (ret) {
    printk(KERN_ERR "%s: Failed to request GPIO pin %d\n", DEVICE_NAME,
           gpio_pin);
    return ret;
  }

  ret = gpio_direction_output(gpio_pin, 0); // Set as output, initial state off
  if (ret) {
    printk(KERN_ERR "%s: Failed to set GPIO direction\n", DEVICE_NAME);
    gpio_free(gpio_pin);
    return ret;
  }

  // --- Character Device Registration ---
  major_number = register_chrdev(0, DEVICE_NAME, &fops);
  if (major_number < 0) {
    printk(KERN_ERR "%s: Failed to register a major number\n", DEVICE_NAME);
    gpio_free(gpio_pin);
    return major_number;
  }
  printk(KERN_INFO "%s: Registered correctly with major number %d\n",
         DEVICE_NAME, major_number);

  // --- Class Creation ---
  device_class = class_create(THIS_MODULE, DEVICE_NAME);
  if (IS_ERR(device_class)) {
    printk(KERN_ERR "%s: Failed to register device class\n", DEVICE_NAME);
    unregister_chrdev(major_number, DEVICE_NAME);
    gpio_free(gpio_pin);
    return PTR_ERR(device_class);
  }

  // --- Device Creation ---
  device = device_create(device_class, NULL, MKDEV(major_number, 0), NULL,
                         DEVICE_NAME);
  if (IS_ERR(device)) {
    printk(KERN_ERR "%s: Failed to create device\n", DEVICE_NAME);
    class_destroy(device_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    gpio_free(gpio_pin);
    return PTR_ERR(device);
  }

  printk(KERN_INFO "%s: Device class created correctly\n", DEVICE_NAME);
  return 0; // OK
}

static void __exit my_module_exit(void) {
  // Unregister platform driver
  platform_driver_unregister(&gpio_led_driver);

  // Destroy device and class
  device_destroy(device_class, MKDEV(major_number, 0));
  class_destroy(device_class);

  // Unregister character device
  unregister_chrdev(major_number, DEVICE_NAME);

  // Free GPIO
  gpio_set_value(gpio_pin, 0); // Turn off LED
  gpio_free(gpio_pin);

  printk(KERN_INFO "%s: Exiting the LED controller module\n", DEVICE_NAME);
}
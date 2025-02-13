#ifndef GPIO_DEBUGFS_H
#define GPIO_DEBUGFS_H

#include "gpio.h"
#include <linux/debugfs.h>

void led_debugfs_init(struct gpio_led_data *led);
void led_debugfs_remove(struct gpio_led_data *led);

#endif // GPIO_DEBUGFS_H

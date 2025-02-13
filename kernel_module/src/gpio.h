#ifndef GPIO_LED_H
#define GPIO_LED_H

#include <linux/ioctl.h>

#define MAX_GPIOS 8
#define DEVICE_NAME "led_controller"

// IOCTL commands
#define LED_IOC_MAGIC 'L'
#define LED_SET_BRIGHTNESS _IOW(LED_IOC_MAGIC, 1, int)
#define LED_SET_BLINK _IOW(LED_IOC_MAGIC, 2, struct led_blink_params)
#define LED_RESET _IO(LED_IOC_MAGIC, 3)

// Additional IOCTL commands
#define LED_SET_PWM _IOW(LED_IOC_MAGIC, 4, struct pwm_params)
#define LED_GET_STATS _IOR(LED_IOC_MAGIC, 5, struct led_stats)
#define LED_SET_TRIGGER _IOW(LED_IOC_MAGIC, 6, struct trigger_params)
#define LED_SET_THERMAL _IOW(LED_IOC_MAGIC, 7, struct thermal_params)

struct gpio_led_data {
  int gpio_pin;
  int state;
  unsigned int brightness;
  struct timer_list blink_timer;
  bool blinking;
  unsigned int blink_delay_on;
  unsigned int blink_delay_off;
  struct pwm_device *pwm;
  struct led_stats stats;
  struct trigger_params trigger;
  struct thermal_params thermal;
  struct work_struct work;
  struct dentry *debugfs_dir;
  spinlock_t lock;
  bool hardware_pwm;
  bool thermal_shutdown;
  int last_temp;
};

struct led_blink_params {
  unsigned int delay_on;
  unsigned int delay_off;
};

struct pwm_params {
  unsigned int period_ns;
  unsigned int duty_cycle;
  bool hardware_pwm;
};

struct led_stats {
  unsigned long switches;
  unsigned long pwm_changes;
  unsigned long errors;
  unsigned long uptime;
  unsigned long power_cycles;
};

struct trigger_params {
  int gpio_trigger;
  bool rising_edge;
  bool falling_edge;
  unsigned int debounce_ms;
};

struct thermal_params {
  int temp_threshold;
  int hysteresis;
  bool auto_throttle;
};

// Power management states
enum led_power_state { LED_POWER_ON, LED_POWER_SUSPEND, LED_POWER_OFF };

#endif // GPIO_LED_H

#include "gpio_debugfs.h"
#include <linux/debugfs.h>
#include <linux/seq_file.h>

static int stats_show(struct seq_file *s, void *private) {
  struct gpio_led_data *led = s->private;

  seq_printf(s, "Switches: %lu\n", led->stats.switches);
  seq_printf(s, "PWM changes: %lu\n", led->stats.pwm_changes);
  seq_printf(s, "Errors: %lu\n", led->stats.errors);
  seq_printf(s, "Uptime: %lu seconds\n", led->stats.uptime);
  seq_printf(s, "Power cycles: %lu\n", led->stats.power_cycles);
  seq_printf(s, "Temperature: %d°C\n", led->last_temp);
  seq_printf(s, "Thermal shutdown: %s\n", led->thermal_shutdown ? "yes" : "no");

  return 0;
}

static int stats_open(struct inode *inode, struct file *file) {
  return single_open(file, stats_show, inode->i_private);
}

static const struct file_operations stats_fops = {
    .owner = THIS_MODULE,
    .open = stats_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

void led_debugfs_init(struct gpio_led_data *led) {
  led->debugfs_dir = debugfs_create_dir("led_controller", NULL);
  debugfs_create_file("stats", 0444, led->debugfs_dir, led, &stats_fops);
  debugfs_create_bool("hardware_pwm", 0444, led->debugfs_dir,
                      &led->hardware_pwm);
  debugfs_create_u32("temp_threshold", 0644, led->debugfs_dir,
                     &led->thermal.temp_threshold);
}

void led_debugfs_remove(struct gpio_led_data *led) {
  debugfs_remove_recursive(led->debugfs_dir);
}

#include <fcntl.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/led_controller"
#define BUFFER_SIZE 64
#define CONFIG_FILE "led_patterns.json"

typedef struct {
  char *name;
  char *pattern;
  int delay;
} SavedPattern;

void print_menu() {
  printf("\n=== LED Controller Pro Max Ultra ===\n");
  printf("1. Turn LED ON\n");
  printf("2. Turn LED OFF\n");
  printf("3. Basic Blink\n");
  printf("4. Get LED Status\n");
  printf("5. Custom Pattern\n");
  printf("6. Strobe Effect\n");
  printf("7. Morse Code\n");
  printf("8. Save Pattern\n");
  printf("9. Load Pattern\n");
  printf("0. System Monitor Mode\n");
  printf("q. Quit\n");
  printf("Choose an option: ");
}

void blink_led(int fd, int times, int delay_ms) {
  char cmd;
  for (int i = 0; i < times; i++) {
    cmd = '1';
    write(fd, &cmd, 1);
    usleep(delay_ms * 1000);
    cmd = '0';
    write(fd, &cmd, 1);
    usleep(delay_ms * 1000);
  }
}

void strobe_effect(int fd, int intensity) {
  char cmd;
  for (int i = 0; i < intensity; i++) {
    cmd = '1';
    write(fd, &cmd, 1);
    usleep(50000); // 50ms
    cmd = '0';
    write(fd, &cmd, 1);
    usleep(20000); // 20ms
  }
}

void morse_code(int fd, const char *text) {
  const char *morse[] = {".-",   "-...", "-.-.", "-..",  ".",    "..-.", "--.",
                         "....", "..",   ".---", "-.-",  ".-..", "--",   "-.",
                         "---",  ".--.", "--.-", ".-.",  "...",  "-",    "..-",
                         "...-", ".--",  "-..-", "-.--", "--.."};

  for (int i = 0; text[i]; i++) {
    if (text[i] >= 'a' && text[i] <= 'z') {
      const char *code = morse[text[i] - 'a'];
      for (int j = 0; code[j]; j++) {
        char cmd = '1';
        write(fd, &cmd, 1);
        usleep(code[j] == '.' ? 100000 : 300000);
        cmd = '0';
        write(fd, &cmd, 1);
        usleep(100000);
      }
      usleep(300000); // Letter spacing
    }
  }
}

void save_pattern(const char *name, const char *pattern, int delay) {
  json_object *root = json_object_new_object();
  json_object *patterns = json_object_new_array();

  // Load existing patterns
  FILE *f = fopen(CONFIG_FILE, "r");
  if (f) {
    // ...load existing patterns...
    fclose(f);
  }

  // Add new pattern
  json_object *new_pattern = json_object_new_object();
  json_object_object_add(new_pattern, "name", json_object_new_string(name));
  json_object_object_add(new_pattern, "pattern",
                         json_object_new_string(pattern));
  json_object_object_add(new_pattern, "delay", json_object_new_int(delay));
  json_object_array_add(patterns, new_pattern);

  // Save to file
  f = fopen(CONFIG_FILE, "w");
  fprintf(f, "%s",
          json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
  fclose(f);
}

void system_monitor_mode(int fd) {
  char cmd;
  FILE *cpu_info = fopen("/proc/stat", "r");
  while (1) {
    // Read CPU usage and blink LED accordingly
    unsigned long long user, nice, system, idle;
    fscanf(cpu_info, "cpu %llu %llu %llu %llu", &user, &nice, &system, &idle);
    float cpu_usage =
        (float)(user + nice + system) / (user + nice + system + idle) * 100;

    if (cpu_usage > 80) {
      strobe_effect(fd, 5); // High CPU alert
    } else if (cpu_usage > 50) {
      cmd = '1';
      write(fd, &cmd, 1);
      usleep(200000);
      cmd = '0';
      write(fd, &cmd, 1);
      usleep(200000);
    }

    rewind(cpu_info);
    if (getchar() == 'q')
      break;
  }
  fclose(cpu_info);
}

int main() {
  int fd;
  char input[BUFFER_SIZE];
  char cmd;
  char status;

  fd = open(DEVICE_PATH, O_RDWR);
  if (fd < 0) {
    perror("Failed to open the device");
    return 1;
  }

  printf("LED Controller Pro Max Started!\n");

  while (1) {
    print_menu();
    fgets(input, BUFFER_SIZE, stdin);

    switch (input[0]) {
    case '1':
      cmd = '1';
      write(fd, &cmd, 1);
      printf("LED turned ON\n");
      break;

    case '2':
      cmd = '0';
      write(fd, &cmd, 1);
      printf("LED turned OFF\n");
      break;

    case '3':
      printf("Enter number of blinks: ");
      fgets(input, BUFFER_SIZE, stdin);
      int blinks = atoi(input);
      printf("Enter delay (ms): ");
      fgets(input, BUFFER_SIZE, stdin);
      int delay = atoi(input);
      blink_led(fd, blinks, delay);
      break;

    case '4':
      if (read(fd, &status, 1) > 0) {
        printf("LED Status: %s\n", status == '1' ? "ON" : "OFF");
      } else {
        printf("Failed to read LED status\n");
      }
      break;

    case '5':
      printf("Enter pattern (e.g., 1010): ");
      fgets(input, BUFFER_SIZE, stdin);
      for (int i = 0; input[i] != '\n' && input[i] != '\0'; i++) {
        if (input[i] == '1' || input[i] == '0') {
          write(fd, &input[i], 1);
          usleep(500000); // 500ms delay
        }
      }
      break;

    case '6':
      printf("Enter strobe intensity (1-10): ");
      fgets(input, BUFFER_SIZE, stdin);
      strobe_effect(fd, atoi(input));
      break;

    case '7':
      printf("Enter text for morse code: ");
      fgets(input, BUFFER_SIZE, stdin);
      input[strcspn(input, "\n")] = 0;
      morse_code(fd, input);
      break;

    case '8':
      printf("Enter pattern name: ");
      char name[32], pattern[64];
      fgets(name, 32, stdin);
      printf("Enter pattern: ");
      fgets(pattern, 64, stdin);
      printf("Enter delay (ms): ");
      fgets(input, BUFFER_SIZE, stdin);
      save_pattern(name, pattern, atoi(input));
      break;

    case '9':
      // Pattern loading logic
      break;

    case '0':
      system_monitor_mode(fd);
      break;

    case 'q':
      close(fd);
      printf("Goodbye!\n");
      return 0;

    default:
      printf("Invalid option!\n");
    }
  }

  return 0;
}

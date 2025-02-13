# GPIO LED Controller Pro Max Ultra

## Overview

A comprehensive Linux kernel module and user-space application for controlling LEDs on Raspberry Pi 5. This project demonstrates hardware control through kernel drivers with a feature-rich command-line interface.

## Features

- LED ON/OFF control
- Customizable LED blinking patterns
- Real-time LED status monitoring
- Multiple blink modes with adjustable timing
- User-friendly command-line interface

## Advanced Features

- **Pattern Effects**

  - Strobe lighting with adjustable intensity
  - Morse code text-to-LED conversion
  - Custom pattern saving and loading
  - System resource monitoring mode

- **Smart Controls**
  - JSON-based pattern storage
  - CPU usage monitoring
  - Pattern library management
  - Advanced timing controls

## Requirements

- Raspberry Pi 5
- Linux kernel headers
- GCC compiler
- CMake (version 3.10 or higher)
- Make

## Installation

### 1. Clone the Repository

```bash
git clone https://github.com/soc-pi/AdvancedGPIOLEDController.git
cd AdvancedGPIOLEDController
```

### 2. Build Kernel Module

```bash
cd kernel_module
make clean
make
sudo insmod gpio.ko
```

### 3. Build Application

```bash
cd ../application
mkdir build && cd build
cmake ..
make
```

## Usage

### LED Controller Pro Max Commands

1. **Basic Control**

   - Option 1: Turn LED ON
   - Option 2: Turn LED OFF
   - Option 4: Check LED Status

2. **Blink Modes**

   - Option 3: Configurable blink
     - Set number of blinks
     - Set delay between blinks
   - Option 5: Custom pattern mode
     - Enter pattern (e.g., 1010 for alternate blinking)

3. **Ultra Features**

   - Option 6: Strobe Effect
     - Enter intensity (1-10)
     - Higher values create faster strobing
   - Option 7: Morse Code
     - Enter text to convert to morse
     - LED will blink corresponding pattern
   - Option 8: Save current pattern
   - Option 9: Load saved pattern
   - Option 0: System Monitor Mode
     - LED indicates system load:
       - Steady: Normal load
       - Fast blink: High CPU usage
       - Strobe: Critical load

4. **Exit**
   - 'q': Quit application

### Example Usage

```bash
./led_controller_pro
# Follow the on-screen menu to control the LED
```

## Hardware Connection

Connect your LED to the following GPIO pins:

- Positive (Anode) → GPIO18 (Pin 12)
- Negative (Cathode) → GND (Pin 6)

## Troubleshooting

1. **Device Not Found**

   - Ensure kernel module is loaded: `lsmod | grep gpio`
   - Check device permissions: `ls -l /dev/led_controller`

2. **LED Not Responding**
   - Verify LED polarity
   - Check physical connections
   - Ensure proper GPIO pin configuration

## Development

### Project Structure

```
gpio/
├── kernel_module/          # Kernel driver implementation
│   ├── src/
│   │   └── gpio.c         # Driver source code
│   ├── include/
│   │   └── gpio.h         # Driver headers
│   └── Makefile
└── application/           # User-space application
    ├── main.c            # LED controller interface
    └── CMakeLists.txt
```

### GPIO Pin Reference

| Pin Number | Name   | Function          |
| ---------- | ------ | ----------------- |
| 12         | GPIO18 | LED Control (Out) |
| 6          | GND    | Ground            |

For full GPIO pinout, see [Raspberry Pi 5 Documentation](https://www.raspberrypi.com/documentation/)

## Pattern Library

Patterns are stored in `led_patterns.json`:

```json
{
  "patterns": [
    {
      "name": "alert",
      "pattern": "10101010",
      "delay": 100
    }
  ]
}
```

## Contributing

1. Fork the repository
2. Create your feature branch
3. Submit a pull request

## License

This project is licensed under the GPL License - see the LICENSE file for details.

## Authors

- Your Name
- Contributors welcome!

# gpio

## Description
A simple kernel module with a user-space application.

## Project Structure
```
gpio/
├── kernel_module/
│   ├── src/
│   │   └── gpio.c
│   ├── include/
│   │   └── gpio.h
│   └── Makefile
└── application/
    ├── main.c
    └── CMakeLists.txt
```

## Build

### Kernel Module
```bash
cd gpio/kernel_module
make
```

### Application
```bash
cd gpio/application
cmake .
make
```

## Usage

### Kernel Module
```bash
sudo insmod gpio/kernel_module/gpio.ko
# Do something with the module
sudo rmmod gpio
```

### Application
```bash
cd gpio/application
./test_app
```

## License
GPL

# Makefile for the ${PROJECT_NAME} kernel module

obj-m += ${PROJECT_NAME}.o
obj-m += gpio.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

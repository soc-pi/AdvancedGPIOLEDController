# Makefile for the ${PROJECT_NAME} kernel module

obj-m += ${PROJECT_NAME}.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/kernel_module modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/kernel_module clean

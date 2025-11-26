ifneq ($(KERNELRELEASE),)
    # Kernel build system sees this part
    obj-m := xpad.o 

else
    # Paths for cross-compiling on EC535 environment
    # Requires kernel that includes framebuffer module (fb)!
    KERNELDIR := $(EC535)/bbb/stock/stock-linux-4.19.82-ti-rt-r33-fb
    PWD := $(shell pwd)
    ARCH := arm
    CROSS := arm-linux-gnueabihf-

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=$(ARCH) clean
endif

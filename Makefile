ifeq ($(KERNELRELEASE),)

ifeq ($(ARCH),arm)
KERNELDIR ?= /home/pi/linux
ROOTFS ?= /lib/modules/$(shell uname -r)
else
# KERNELDIR ?= /lib/modules/$(shell uname -r)/build
endif
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	dtc -I dts -O dtb -o simple_device.dtbo simple_device.dts
modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) INSTALL_MOD_PATH=$(ROOTFS) modules_install
clean:
	rm -rf *.o *.ko .*.cmd *.mod.* modules.order Module.symvers .tmp_versions
else

obj-m := simple_drv.o

endif
HOST_ARCH        ?= $(shell uname -m | sed -e s/arm.*/arm/ -e s/aarch64.*/arm64/)
ARCH             ?= arm64
KERNEL_SRC_DIR   ?= ../linux-headers-4.14.0-xlnx-v2018.2-zynqmp-fpga
XLNX_DRV_HDR_DIR ?= ../../../ultra96_design/ultra96_design.sdk/standalone_bsp_0/psu_cortexa53_0/include/

ifeq ($(ARCH), arm)
 ifneq ($(HOST_ARCH), arm)
   CROSS_COMPILE  ?= arm-linux-gnueabihf-
 endif
endif
ifeq ($(ARCH), arm64)
 ifneq ($(HOST_ARCH), arm64)
   CROSS_COMPILE  ?= aarch64-linux-gnu-
 endif
endif

obj-m := zynq_v4l2.o

ccflags-y += -I $(XLNX_DRV_HDR_DIR)

all:
	make -C $(KERNEL_SRC_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules

clean:
	make -C $(KERNEL_SRC_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) clean


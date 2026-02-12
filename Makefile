KDIR := /lib/modules/$(shell uname -r)/build
MODULE_DIR := $(PWD)
MODULE_OUTPUT_DIR := $(PWD)/build

obj-m := cw2217-battery.o

all:
	mkdir -p $(MODULE_OUTPUT_DIR)
	$(MAKE) -C $(KDIR) M=$(MODULE_DIR) MO=$(MODULE_OUTPUT_DIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(MODULE_DIR) MO=$(MODULE_OUTPUT_DIR) clean
	rm -rf $(MODULE_OUTPUT_DIR)

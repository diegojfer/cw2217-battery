KDIR := /lib/modules/$(shell uname -r)/build
MODULE_DIR := $(PWD)
MODULE_OUTPUT_DIR := $(PWD)/build

obj-m := cw2217b.o

all:
	mkdir -p $(MODULE_OUTPUT_DIR)
	$(MAKE) -C $(KDIR) M=$(MODULE_DIR) MO=$(MODULE_OUTPUT_DIR) modules
	dtc -@ -I dts -O dtb -o $(MODULE_OUTPUT_DIR)/cw2217b.dtbo cw2217b.dts

clean:
	$(MAKE) -C $(KDIR) M=$(MODULE_DIR) MO=$(MODULE_OUTPUT_DIR) clean
	rm -rf $(MODULE_OUTPUT_DIR)

# MorseMouse out-of-tree kernel module.

.PHONY: all modules_install clean

ifneq ($(KERNELRELEASE),)

# Linux Kbuild section.
obj-m += hid_morse_kbd.o

else

# External module build entry points.
all: modules

modules:
	$(MAKE) -C "$(KERNEL_SRC)" O="$(O)" M="$(CURDIR)" modules

modules_install:
	$(MAKE) -C "$(KERNEL_SRC)" O="$(O)" M="$(CURDIR)" modules_install

clean:
	$(MAKE) -C "$(KERNEL_SRC)" O="$(O)" M="$(CURDIR)" clean

endif

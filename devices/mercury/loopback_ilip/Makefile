KDIR := /lib/modules/$(shell uname -r)/build

obj-m += gaps_ilip.o

MY_CFLAGS += -g -DDEBUG
ccflags-y += ${MY_CFLAGS}
CC += ${MY_CFLAGS}

gaps_ilip-objs := ilip_base.o

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

debug:
	$(MAKE) EXTRA_CFLAGS="$(MY_CFLAGS)" -C $(KDIR) SUBDIRS=$(PWD) modules 

clean:
	rm -rf *.o *.ko *.mod.* *.cmd .module* modules* Module* .*.cmd .tmp*
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


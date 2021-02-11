KDIR := /lib/modules/$(shell uname -r)/build

obj-m += gaps_ilip.o gaps_loopback.o

MY_CFLAGS += -g -DDEBUG
ccflags-y += ${MY_CFLAGS}
CC += ${MY_CFLAGS}

gaps_ilip-objs := ilip_common.o ilip_loopback.o ilip_nl.o

gaps_loopback-objs := ilip_common.o ilip_base.o ilip_nl.o

all:
	$(MAKE) -C $(KDIR) M=$(shell pwd) modules

debug:
	$(MAKE) EXTRA_CFLAGS="$(MY_CFLAGS)" -C $(KDIR) M=$(shell pwd) modules 

clean:
	rm -rf *.o *.ko *.mod.* *.cmd .module* modules* Module* .*.cmd .tmp*
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean


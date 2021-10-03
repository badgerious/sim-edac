obj-m += sim_edac.o

PWD := $(shell pwd)
KDIR ?= /lib/modules/$(shell uname -r)/build
ccflags-y += -I$(KDIR)/drivers/edac

default:
	$(MAKE) -C $(KDIR) $(MAKE_OPTS) M=$(PWD)

clean:
	$(MAKE) -C $(KDIR) $(MAKE_OPTS) M=$(PWD) clean

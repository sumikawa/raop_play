KERNEL_VERSION=$(shell uname -r)
KERNEL_SRC=$(TARGET_TOP)/lib/modules/$(KERNEL_VERSION)/build
MOD_INSTALL_DIR=$(TARGET_TOP)/lib/modules/$(KERNEL_VERSION)/sound
target_module := alsa_raoppcm
obj-m := $(target_module).o

KERNEL_2_4=$(findstring 2.4.,$(KERNEL_VERSION))
KERNEL_2_6=$(findstring 2.6.,$(KERNEL_VERSION))

$(KERNEL_2_6): 
	$(KERNEL_COMPILE_OPTIONS) make -C $(KERNEL_SRC)  SUBDIRS=$(shell pwd) modules

$(KERNEL_2_4): $(target_module).c
	echo "kernel 2.4 is not yet supported"
	exit 1

install:
	install -d $(MOD_INSTALL_DIR)
	cp -f $(target_module).ko $(MOD_INSTALL_DIR) || exit 1
	$(GCC_PREFIX)strip -g $(MOD_INSTALL_DIR)/$(target_module).ko || exit 1
	depmod

clean:
	rm -f *.o *~ .*.cmd *.ko *.mod.c
#############################################
# To compile test programs
#############################################

APTARGETS := pcm_test

# set the default objects as TARGET.o for each TARGET
$(foreach i,$(APTARGETS),$(eval $(i)_OBJS:=$(i).o))
# override each *_OBJS here if they are not default

ifndef APTARGET_OBJS
.PHONY: appli $(APTARGETS)
endif

APLIBS=-lasound
APCFLAGS=-Wall -g

appli: $(APTARGETS)

$(APTARGETS): $($(APTARGET_OBJS))
	if [ -z "$(APTARGET_OBJS)" ]; then \
		CFLAGS="$(APCFLAGS)" APTARGET_OBJS=$@_OBJS make -C . $@; \
	else \
		$(CC) $^ -o $@ $(APLIBS); \
	fi

apclean:
	rm -f $(APTARGETS) $(foreach i,$(APTARGETS),$($(i)_OBJS))




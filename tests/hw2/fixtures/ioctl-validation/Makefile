PROGRAM := hw2-ioctl-validation
MINEMU ?= minemu
ROOT := ../../../..
IMAGE := build/hw2-ioctl-validation.img

include $(ROOT)/user/common/common.mk

LDFLAGS := $(subst $(USER_DIR)/common/user.ld,user.ld,$(LDFLAGS))
$(ELF): user.ld

.PHONY: kernel image

all: image

kernel:
	$(MAKE) -C $(ROOT)/kernel all

image: $(IMAGE)

$(IMAGE): image.toml $(ELF) kernel
	@mkdir -p $(dir $@)
	$(MINEMU) image image.toml --output $@

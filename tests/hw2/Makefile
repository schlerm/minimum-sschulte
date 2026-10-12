MINEMU ?= minemu

.PHONY: all prepare-hello prepare-devices prepare-ioctl-validation clean

all: prepare-hello prepare-devices prepare-ioctl-validation

prepare-hello:
	$(MAKE) -C fixtures/hello MINEMU="$(MINEMU)" all

prepare-devices:
	$(MAKE) -C fixtures/devices MINEMU="$(MINEMU)" all

prepare-ioctl-validation:
	$(MAKE) -C fixtures/ioctl-validation MINEMU="$(MINEMU)" all

prepare-%:
	@:

clean:
	$(MAKE) -C fixtures/hello clean
	$(MAKE) -C fixtures/devices clean
	$(MAKE) -C fixtures/ioctl-validation clean

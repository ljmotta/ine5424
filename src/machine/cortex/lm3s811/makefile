# EPOS Cortex Machine Makefile

include ../../../../makedefs

OBJS := $(subst .cc,.o,$(shell find *.cc | grep -v _test | grep -v _init))
INITS := $(subst .cc,.o,$(shell find *.cc | grep _init))

all:		$(LIBINIT) #$(LIBMACH)

$(LIBMACH):	$(LIBMACH)($(OBJS))

$(LIBINIT):	$(LIBINIT)($(INITS))

clean:
		$(CLEAN) *.o *_test

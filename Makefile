
CFLAGS := $(shell pkg-config --cflags libdrm egl glesv2)

LDFLAGS := $(shell pkg-config --libs libdrm egl glesv2)

TARGETS := drmAlphaTest

CXX ?= g++

CFLAGS += -g3 -O0

$(info CFLAGS=$(CFLAGS) )
$(info LDFLAGS=$(LDFLAGS) )

all: $(TARGETS)

clean:
	rm -fv $(TARGETS) *.o *.d compile_commands.json

OBJECTS_drmAlphaTest = drmAlphaTest.o

.cpp.o:
	$(CXX) -c $(CFLAGS) -o $@ -MD -MP $<

drmAlphaTest: $(OBJECTS_drmAlphaTest)
	$(CXX) $(CFLAGS) $? $(LDFLAGS) -o $@
	
include $(wildcard *.d)

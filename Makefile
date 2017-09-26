TOPDIR  := $(shell cd ..; pwd)
# From d2xx/examples/Rules.make
DEPENDENCIES := -lftd2xx -lpthread

UNAME := $(shell uname)
# Assume target is Mac OS if build host is Mac OS; any other host targets Linux
ifeq ($(UNAME), Darwin)
	DEPENDENCIES += -lobjc -framework IOKit -framework CoreFoundation
else
	DEPENDENCIES += -lrt
endif

# Embed in the executable a run-time path to libftd2xx
LINKER_OPTIONS := -Wl,-rpath /usr/local/lib

INCLUDE_DIR := $(D2XX_ROOT)

CFLAGS = -Wall -Wextra $(DEPENDENCIES) -I$(INCLUDE_DIR) $(LINKER_OPTIONS) -L/usr/local/lib
# End snippet from d2xx/examples/Rules.make

APP = bitmode

all: $(APP)

$(APP): main.c
	$(CC) main.c -o $(APP) $(CFLAGS)

clean:
	-rm -f *.o ; rm $(APP)

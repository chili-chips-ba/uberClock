BUILD_DIR?=../build/

include $(BUILD_DIR)/software/include/generated/variables.mak
include $(SOC_DIRECTORY)/software/common.mak

OBJECTS = src/donut.o src/helloc.o crt0.o src/main.o src/console.o src/ubddr3.o src/kiss_fft.o \
          src/uberclock.o \
          src/uberclock_common.o \
          src/uberclock_cmds.o \
          src/uberclock_dsp.o \
          src/uberclock_capture.o \
          src/uberclock_udp.o

CFLAGS += -I$(CURDIR)
CFLAGS += -I$(CURDIR)/inc
CFLAGS += -I$(CURDIR)/kissfft
CFLAGS += -DFIXED_POINT=16
ifdef WITH_CXX
	OBJECTS += src/hellocpp.o
	CFLAGS += -DWITH_CXX
endif


all: demo.bin


%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
ifneq ($(OS),Windows_NT)
	chmod -x $@
endif

vpath %.a $(PACKAGES:%=../%)

demo.elf: $(OBJECTS)
	$(CC) $(LDFLAGS) -T linker.ld -N -o $@ \
		$(OBJECTS) \
		$(PACKAGES:%=-L$(BUILD_DIR)/software/%) \
		-Wl,--whole-archive \
		-Wl,--gc-sections \
		-Wl,-Map,$@.map \
		$(LIBS:lib%=-l%)

ifneq ($(OS),Windows_NT)
	chmod -x $@
endif

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.o=.d)

src/donut.o: CFLAGS   += -w

VPATH = $(BIOS_DIRECTORY):$(BIOS_DIRECTORY)/cmds:$(CPU_DIRECTORY)


%.o: %.cpp
	$(compilexx)

%.o: %.c
	$(compile)

%.o: %.S
	$(assemble)

clean:
	$(RM) $(OBJECTS) demo.elf demo.bin .*~ *~

.PHONY: all clean

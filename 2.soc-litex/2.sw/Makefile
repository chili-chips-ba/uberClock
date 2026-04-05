BUILD_DIR?=../build/

include $(BUILD_DIR)/software/include/generated/variables.mak
include $(SOC_DIRECTORY)/software/common.mak

OBJECTS   = ascii_donut.o hello_c.o crt0.o main.o console.o \
	uberclock.o cli.o basic.o fft.o siggen.o dma.o regs.o runtime.o \
	ubddr3.o kiss_fft.o
CFLAGS   += -I$(CURDIR)
CFLAGS   += -I$(CURDIR)/inc
CFLAGS   += -I$(CURDIR)/kissfft
CFLAGS   += -DFIXED_POINT=16
ifdef WITH_CXX
	OBJECTS += hello_cpp.o
	CFLAGS += -DWITH_CXX
endif


all: demo.bin


%.bin: %.elf
	@$(OBJCOPY) -O binary $< $@
ifneq ($(OS),Windows_NT)
	@chmod -x $@
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
	@chmod -x $@
endif

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.o=.d)

ascii_donut.o: CFLAGS   += -w

VPATH = $(BIOS_DIRECTORY):$(BIOS_DIRECTORY)/cmds:$(CPU_DIRECTORY):$(CURDIR)/src/uberclock


%.o: %.cpp
	$(compilexx)

%.o: %.c
	$(compile)

%.o: %.S
	$(assemble)

clean:
	@printf " CLEAN    %s\n" "$(CURDIR)"
	@$(RM) $(OBJECTS) demo.elf demo.bin .*~ *~ \
		donut.o donut.d helloc.o helloc.d hellocpp.o hellocpp.d \
		uberclock_cli.o uberclock_cli.d uberclock_basic.o uberclock_basic.d \
		uberclock_fft.o uberclock_fft.d uberclock_siggen.o uberclock_siggen.d \
		uberclock_dma.o uberclock_dma.d uberclock_regs.o uberclock_regs.d \
		uberclock_runtime.o uberclock_runtime.d

.PHONY: all clean

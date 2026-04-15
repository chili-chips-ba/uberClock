PACKAGES=libc libcompiler_rt libbase libfatfs liblitespi liblitedram libliteeth liblitesdcard liblitesata bios
PACKAGE_DIRS=/home/ahmed/litex/litex/litex/soc/software/libc /home/ahmed/litex/litex/litex/soc/software/libcompiler_rt /home/ahmed/litex/litex/litex/soc/software/libbase /home/ahmed/litex/litex/litex/soc/software/libfatfs /home/ahmed/litex/litex/litex/soc/software/liblitespi /home/ahmed/litex/litex/litex/soc/software/liblitedram /home/ahmed/litex/litex/litex/soc/software/libliteeth /home/ahmed/litex/litex/litex/soc/software/liblitesdcard /home/ahmed/litex/litex/litex/soc/software/liblitesata /home/ahmed/litex/litex/litex/soc/software/bios
LIBS=libc libcompiler_rt libbase libfatfs liblitespi liblitedram libliteeth liblitesdcard liblitesata
TRIPLE=riscv64-unknown-elf
CPU=vexriscv
CPUFAMILY=riscv
CPUFLAGS=-march=rv32i2p0_m     -mabi=ilp32 -D__vexriscv__
CPUENDIANNESS=little
CLANG=0
CPU_DIRECTORY=/home/ahmed/litex/litex/litex/soc/cores/cpu/vexriscv
SOC_DIRECTORY=/home/ahmed/litex/litex/litex/soc
PICOLIBC_DIRECTORY=/home/ahmed/litex/pythondata-software-picolibc/pythondata_software_picolibc/data
PICOLIBC_FORMAT=integer
COMPILER_RT_DIRECTORY=/home/ahmed/litex/pythondata-software-compiler_rt/pythondata_software_compiler_rt/data
export BUILDINC_DIRECTORY
BUILDINC_DIRECTORY=/home/ahmed/ws/uberClock/3.build/build/alinx_ax7203/software/include
LIBC_DIRECTORY=/home/ahmed/litex/litex/litex/soc/software/libc
LIBCOMPILER_RT_DIRECTORY=/home/ahmed/litex/litex/litex/soc/software/libcompiler_rt
LIBBASE_DIRECTORY=/home/ahmed/litex/litex/litex/soc/software/libbase
LIBFATFS_DIRECTORY=/home/ahmed/litex/litex/litex/soc/software/libfatfs
LIBLITESPI_DIRECTORY=/home/ahmed/litex/litex/litex/soc/software/liblitespi
LIBLITEDRAM_DIRECTORY=/home/ahmed/litex/litex/litex/soc/software/liblitedram
LIBLITEETH_DIRECTORY=/home/ahmed/litex/litex/litex/soc/software/libliteeth
LIBLITESDCARD_DIRECTORY=/home/ahmed/litex/litex/litex/soc/software/liblitesdcard
LIBLITESATA_DIRECTORY=/home/ahmed/litex/litex/litex/soc/software/liblitesata
BIOS_DIRECTORY=/home/ahmed/litex/litex/litex/soc/software/bios
LTO=0
BIOS_CONSOLE_FULL=1
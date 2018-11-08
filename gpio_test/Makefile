all:
	@scons -j16 --buildlib=libkernel
	@scons -j16 --buildlib=libdriver
	@scons -j16

menuconfig:
	@scons --menuconfig

clean:
	@scons -c
	@rm -rf build
	@rm -f rtthread.elf rtthread.bin rtthread.map rtthread.dis rtthread.s
	@rm -f rtt_stage[12].img
	@rm -f lib*_gcc.a

distclean: clean
	@rm -f rtconfig_kernel.h
	@rm -f rtconfig_driver.h
	@rm -f rtconfig_platform.h
	@rm -f .config
	@rm -f flash.img

%_defconfig:
	@scons --defconfig=$@

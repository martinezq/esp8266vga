#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := vga1

include $(IDF_PATH)/make/project.mk

dasm:
	mkdir -p tmp
	xtensa-lx106-elf-objdump -d build/main/vga1_main.o > tmp/vga1.asm
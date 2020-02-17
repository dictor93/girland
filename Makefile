# Makefile for the ws2812_i2s example

PROGRAM=girland
EXTRA_COMPONENTS = extras/i2s_dma extras/ws2812_i2s extras/dhcpserver extras/spiffs
PROGRAM_SRC_DIR=. ./src
PROGRAM_INC_DIR = ./inc
PROGRAM_SRC_FILES=./girland.c
FLASH_SIZE = 4


# SPIFFS_BASE_ADDR = 0x70000
# SPIFFS_SIZE = 0x10000

include ../../common.mk

# $(eval $(call make_spiffs_image,files))

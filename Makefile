###############################################################################
#
# makefile for vbcc or gcc
#
# original author: Henryk Richter <henryk.richter@gmx.net>
# 
# concept:
#
# tools required:
#  - vbcc, defaulting to m68k-amigaos
#  - vlink
#  (- vasm)
#
# porting:
#
#  see Common.mk
#
###############################################################################

###############################################################################
# Date, version, extra objects to build
# 
###############################################################################
DEVICEVERSION=1
DEVICEREVISION=5
DEVICEDATE=2026-03-09

###############################################################################
# Devices to build (1 or 2, keep DEVICEID2 empty if only one build is desired)
#
###############################################################################

DEVICEID=scsidayna.device
DEFINES = #
ASMDEFS = #
CPU = 68000

DEVICEID2=

###############################################################################
# import generic ruleset
# 
###############################################################################
include Common.mk

# the Shell tool (vbcc, same flags as the device): make daynawifi
daynawifi: daynawifi.c scsiwifi.c scsiwifi.h
	vc +aos68k -O2 -+ -sc -cpu=$(CPU) $(IPATH) -I. -o $@ daynawifi.c scsiwifi.c -lamiga

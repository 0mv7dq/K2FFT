# pd-lib-builder, with PUREfft from a PUREfft.pc: make PUREFFT=<the folder holding PUREfft.pc> PDINCLUDEDIR=<m_pd.h's folder>
lib.name = fftcheck
class.sources = fftcheck.c
cflags += $(shell PKG_CONFIG_PATH=$(PUREFFT) pkg-config --cflags PUREfft)
ldlibs += $(shell PKG_CONFIG_PATH=$(PUREFFT) pkg-config --libs PUREfft)
include Makefile.pdlibbuilder

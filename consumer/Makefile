# pd-lib-builder, with K2FFT from a K2FFT.pc: make K2FFT=<the folder holding K2FFT.pc> PDINCLUDEDIR=<m_pd.h's folder>
lib.name = fftcheck
class.sources = fftcheck.c
cflags += $(shell PKG_CONFIG_PATH=$(K2FFT) pkg-config --cflags K2FFT)
ldlibs += $(shell PKG_CONFIG_PATH=$(K2FFT) pkg-config --libs K2FFT)
include Makefile.pdlibbuilder

#############################################################################
# Makefile for building: hdspmidi
# Project:  hdspmidi.pro
# Template: app
# Command: /usr/bin/qmake -o Makefile hdspmidi.pro
#############################################################################

####### Compiler, tools and options

CC            = gcc
CXX           = g++
DEFINES       = 
CFLAGS        = -pipe -g -O0 -Wall -W  $(DEFINES)
CXXFLAGS      = -pipe -g -O0 -Wall -W  $(DEFINES)
INCPATH       = -Isrc -I.
LINK          = g++
LFLAGS        = -Wl,-O1
LIBS          = $(SUBLIBS)   -lasound -lconfig++ -lserial -ludev
AR            = ar cqs
RANLIB        = 
TAR           = tar -cf
COMPRESS      = gzip -9f
COPY          = cp -f
SED           = sed
COPY_FILE     = $(COPY)
COPY_DIR      = $(COPY) -r
STRIP         = strip
INSTALL_FILE  = install -m 644 -p
INSTALL_DIR   = $(COPY_DIR)
INSTALL_PROGRAM = install -m 755 -p
DEL_FILE      = rm -f
SYMLINK       = ln -f -s
DEL_DIR       = rmdir
MOVE          = mv -f
CHK_DIR_EXISTS= test -d
MKDIR         = mkdir -p

####### Output directory

OBJECTS_DIR   = build/

####### Files

SOURCES       = src/bridge.cpp \
		src/Channel.cpp \
		src/channelmap.cxx \
		src/HDSPMixerCard.cxx \
		src/midi.cpp \
		src/midicontroller.cpp \
		src/relay.cpp 
OBJECTS       = build/bridge.o \
		build/Channel.o \
		build/channelmap.o \
		build/HDSPMixerCard.o \
		build/midi.o \
		build/midicontroller.o \
		build/relay.o
DESTDIR       = 
TARGET        = hdspmidi

first: all
####### Implicit rules

.SUFFIXES: .o .c .cpp .cc .cxx .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o "$@" "$<"

####### Build rules

all: Makefile $(TARGET)

$(TARGET):  $(OBJECTS)  
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJCOMP) $(LIBS)

dist: 
	@$(CHK_DIR_EXISTS) build/hdspmidi1.0.0 || $(MKDIR) build/hdspmidi1.0.0 
	$(COPY_FILE) --parents $(SOURCES) $(DIST) build/hdspmidi1.0.0/ && (cd `dirname build/hdspmidi1.0.0` && $(TAR) hdspmidi1.0.0.tar hdspmidi1.0.0 && $(COMPRESS) hdspmidi1.0.0.tar) && $(MOVE) `dirname build/hdspmidi1.0.0`/hdspmidi1.0.0.tar.gz . && $(DEL_FILE) -r build/hdspmidi1.0.0


clean:
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) *~ core *.core


####### Sub-libraries

distclean: clean
	-$(DEL_FILE) $(TARGET) 
	-$(DEL_FILE) Makefile


check: first


####### Compile

build/bridge.o: src/bridge.cpp src/bridge.h \
		src/HDSPMixerCard.h \
		src/channelmap.h \
		src/Channel.h \
		src/midicontroller.h \
		src/relay.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o build/bridge.o src/bridge.cpp

build/Channel.o: src/Channel.cpp src/Channel.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o build/Channel.o src/Channel.cpp

build/channelmap.o: src/channelmap.cxx src/channelmap.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o build/channelmap.o src/channelmap.cxx

build/HDSPMixerCard.o: src/HDSPMixerCard.cxx src/HDSPMixerCard.h \
		src/channelmap.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o build/HDSPMixerCard.o src/HDSPMixerCard.cxx

build/midi.o: src/midi.cpp src/HDSPMixerCard.h \
		src/channelmap.h \
		src/Channel.h \
		src/midicontroller.h \
		src/bridge.h \
		src/relay.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o build/midi.o src/midi.cpp

build/midicontroller.o: src/midicontroller.cpp src/midicontroller.h \
		src/Channel.h \
		src/HDSPMixerCard.h \
		src/channelmap.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o build/midicontroller.o src/midicontroller.cpp

build/relay.o: src/relay.cpp src/relay.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o build/relay.o src/relay.cpp

####### Install

install:   FORCE

uninstall:   FORCE

FORCE:


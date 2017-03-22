#*******************************************************************************
#  Copyright (c) 2009, 2016 IBM Corp.
# 
#  All rights reserved. This program and the accompanying materials
#  are made available under the terms of the Eclipse Public License v1.0
#  and Eclipse Distribution License v1.0 which accompany this distribution. 
# 
#  The Eclipse Public License is available at 
#     http://www.eclipse.org/legal/epl-v10.html
#  and the Eclipse Distribution License is available at 
#    http://www.eclipse.org/org/documents/edl-v10.php.
# 
#  Contributors:
#     Ian Craggs - initial API and implementation and/or initial documentation
#     Allan Stockdill-Mander - SSL updates
#     Andy Piper - various fixes
#     Ian Craggs - OSX build
#     Rainer Poisel - support for multi-core builds and cross-compilation
#*******************************************************************************/

# Note: on OS X you should install XCode and the associated command-line tools

SHELL = /bin/sh
.PHONY: clean, clean-binary, debug, debug-clean, debug-clean-binary, release, release-clean, mkdir, install, uninstall, html

ifndef release.version
  release.version = 1.1.0
endif

# determine current platform
BUILD_TYPE ?= debug
ifeq ($(OS),Windows_NT)
	OSTYPE ?= $(OS)
	MACHINETYPE ?= $(PROCESSOR_ARCHITECTURE)
else
	OSTYPE ?= $(shell uname -s)
	MACHINETYPE ?= $(shell uname -m)
	build.level = $(shell date)
endif # OS
ifeq ($(OSTYPE),linux)
	OSTYPE = Linux
endif

# assume this is normally run in the main Paho directory
ifndef srcdir
  srcdir = paho.mqtt/src
endif

ifndef blddir
  blddir = build/output
endif

ifndef blddir_work
  blddir_work = build
endif

ifndef docdir
  docdir = $(blddir)/doc
endif

ifndef docdir_work
  docdir_work = $(blddir)/../doc
endif

ifndef prefix
	prefix = /usr/local
endif

ifndef exec_prefix
	exec_prefix = ${prefix}
endif

CFLAGS += $(shell pkg-config --cflags json-c)
LDFLAGS += $(shell pkg-config --libs json-c)

bindir = $(exec_prefix)/bin
includedir = $(prefix)/include
libdir = $(exec_prefix)/lib

SOURCE_FILES = $(wildcard $(srcdir)/*.c)
SOURCE_FILES_C = $(filter-out $(srcdir)/MQTTAsync.c $(srcdir)/MQTTVersion.c $(srcdir)/SSLSocket.c, $(SOURCE_FILES))
SOURCE_FILES_CS = $(filter-out $(srcdir)/MQTTAsync.c $(srcdir)/MQTTVersion.c, $(SOURCE_FILES))
SOURCE_FILES_A = $(filter-out $(srcdir)/MQTTClient.c $(srcdir)/MQTTVersion.c $(srcdir)/SSLSocket.c, $(SOURCE_FILES))
SOURCE_FILES_AS = $(filter-out $(srcdir)/MQTTClient.c $(srcdir)/MQTTVersion.c, $(SOURCE_FILES))

HEADERS = $(srcdir)/*.h
HEADERS_C = $(filter-out $(srcdir)/MQTTAsync.h, $(HEADERS))
HEADERS_A = $(HEADERS)

PROGRAM_FILES_C = leader follower
SYNC_PROGRAM = ${addprefix ${blddir}/program/,${PROGRAM_FILES_C}}

# The names of the four different libraries to be built
MQTTLIB_C = paho-mqtt3c
MQTTLIB_CS = paho-mqtt3cs
MQTTLIB_A = paho-mqtt3a
MQTTLIB_AS = paho-mqtt3as

CC = gcc
GCCVERSION = $(shell gcc --version | grep ^gcc | sed 's/^.* //g')
CFLAGS = -g

ifeq "$(GCCVERSION)" "4.9.2"
	CFLAGS += -Wtype-limits
endif

ifndef INSTALL
INSTALL = install
endif
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA =  $(INSTALL) -m 644
DOXYGEN_COMMAND = doxygen

MAJOR_VERSION = 1
MINOR_VERSION = 0
VERSION = ${MAJOR_VERSION}.${MINOR_VERSION}

MQTTLIB_C_TARGET = ${blddir}/lib${MQTTLIB_C}.so.${VERSION}
MQTTLIB_CS_TARGET = ${blddir}/lib${MQTTLIB_CS}.so.${VERSION}
MQTTLIB_A_TARGET = ${blddir}/lib${MQTTLIB_A}.so.${VERSION}
MQTTLIB_AS_TARGET = ${blddir}/lib${MQTTLIB_AS}.so.${VERSION}
MQTTVERSION_TARGET = ${blddir}/MQTTVersion

CCFLAGS_SO = -g -fPIC $(CFLAGS) -Os -Wall -fvisibility=hidden -I$(blddir_work)
FLAGS_EXE = $(LDFLAGS) -I ${srcdir} -lpthread -L ${blddir}
FLAGS_EXES = $(LDFLAGS) -I ${srcdir} ${START_GROUP} -lpthread -lssl -lcrypto ${END_GROUP} -L ${blddir}

LDCONFIG ?= /sbin/ldconfig
LDFLAGS_C = $(LDFLAGS) -shared -Wl,-init,$(MQTTCLIENT_INIT) -lpthread 
LDFLAGS_CS = $(LDFLAGS) -shared $(START_GROUP) -lpthread $(EXTRA_LIB) -lssl -lcrypto $(END_GROUP) -Wl,-init,$(MQTTCLIENT_INIT)
LDFLAGS_A = $(LDFLAGS) -shared -Wl,-init,$(MQTTASYNC_INIT) -lpthread
LDFLAGS_AS = $(LDFLAGS) -shared $(START_GROUP) -lpthread $(EXTRA_LIB) -lssl -lcrypto $(END_GROUP) -Wl,-init,$(MQTTASYNC_INIT)

SED_COMMAND = sed \
    -e "s/@CLIENT_VERSION@/${release.version}/g" \
    -e "s/@BUILD_TIMESTAMP@/${build.level}/g"

ifeq ($(OSTYPE),Linux)

MQTTCLIENT_INIT = MQTTClient_init
MQTTASYNC_INIT = MQTTAsync_init
START_GROUP = -Wl,--start-group
END_GROUP = -Wl,--end-group

EXTRA_LIB = -ldl

LDFLAGS_C += -Wl,-soname,lib$(MQTTLIB_C).so.${MAJOR_VERSION}
LDFLAGS_CS += -Wl,-soname,lib$(MQTTLIB_CS).so.${MAJOR_VERSION} -Wl,-no-whole-archive
LDFLAGS_A += -Wl,-soname,lib${MQTTLIB_A}.so.${MAJOR_VERSION}
LDFLAGS_AS += -Wl,-soname,lib${MQTTLIB_AS}.so.${MAJOR_VERSION} -Wl,-no-whole-archive

else ifeq ($(OSTYPE),Darwin)

MQTTCLIENT_INIT = _MQTTClient_init
MQTTASYNC_INIT = _MQTTAsync_init
START_GROUP =
END_GROUP = 

EXTRA_LIB = -ldl

CCFLAGS_SO += -Wno-deprecated-declarations -DUSE_NAMED_SEMAPHORES
LDFLAGS_C += -Wl,-install_name,lib$(MQTTLIB_C).so.${MAJOR_VERSION}
LDFLAGS_CS += -Wl,-install_name,lib$(MQTTLIB_CS).so.${MAJOR_VERSION}
LDFLAGS_A += -Wl,-install_name,lib${MQTTLIB_A}.so.${MAJOR_VERSION}
LDFLAGS_AS += -Wl,-install_name,lib${MQTTLIB_AS}.so.${MAJOR_VERSION}

endif

all: build

build: | mkdir ${MQTTLIB_C_TARGET} ${MQTTLIB_CS_TARGET} ${MQTTLIB_A_TARGET} ${MQTTLIB_AS_TARGET} ${MQTTVERSION_TARGET} ${SYNC_PROGRAM}


mkdir:
	-mkdir -p ${blddir}/program
	echo OSTYPE is $(OSTYPE)

${SYNC_PROGRAM}: ${blddir}/program/%: ${srcdir}/program/%.c $(MQTTLIB_C_TARGET)
	${CC} -o $@ $< -l${MQTTLIB_C} ${FLAGS_EXE}

$(blddir_work)/VersionInfo.h: $(srcdir)/VersionInfo.h.in
	$(SED_COMMAND) $< > $@

${MQTTLIB_C_TARGET}: ${SOURCE_FILES_C} ${HEADERS_C} $(blddir_work)/VersionInfo.h
	${CC} ${CCFLAGS_SO} -o $@ ${SOURCE_FILES_C} ${LDFLAGS_C}
	-ln -s lib$(MQTTLIB_C).so.${VERSION}  ${blddir}/lib$(MQTTLIB_C).so.${MAJOR_VERSION}
	-ln -s lib$(MQTTLIB_C).so.${MAJOR_VERSION} ${blddir}/lib$(MQTTLIB_C).so

${MQTTLIB_CS_TARGET}: ${SOURCE_FILES_CS} ${HEADERS_C} $(blddir_work)/VersionInfo.h
	${CC} ${CCFLAGS_SO} -o $@ ${SOURCE_FILES_CS} -DOPENSSL ${LDFLAGS_CS}
	-ln -s lib$(MQTTLIB_CS).so.${VERSION}  ${blddir}/lib$(MQTTLIB_CS).so.${MAJOR_VERSION}
	-ln -s lib$(MQTTLIB_CS).so.${MAJOR_VERSION} ${blddir}/lib$(MQTTLIB_CS).so

${MQTTLIB_A_TARGET}: ${SOURCE_FILES_A} ${HEADERS_A} $(blddir_work)/VersionInfo.h
	${CC} ${CCFLAGS_SO} -o $@ ${SOURCE_FILES_A} ${LDFLAGS_A}
	-ln -s lib$(MQTTLIB_A).so.${VERSION}  ${blddir}/lib$(MQTTLIB_A).so.${MAJOR_VERSION}
	-ln -s lib$(MQTTLIB_A).so.${MAJOR_VERSION} ${blddir}/lib$(MQTTLIB_A).so

${MQTTLIB_AS_TARGET}: ${SOURCE_FILES_AS} ${HEADERS_A} $(blddir_work)/VersionInfo.h
	${CC} ${CCFLAGS_SO} -o $@ ${SOURCE_FILES_AS} -DOPENSSL ${LDFLAGS_AS}
	-ln -s lib$(MQTTLIB_AS).so.${VERSION}  ${blddir}/lib$(MQTTLIB_AS).so.${MAJOR_VERSION}
	-ln -s lib$(MQTTLIB_AS).so.${MAJOR_VERSION} ${blddir}/lib$(MQTTLIB_AS).so

${MQTTVERSION_TARGET}: $(srcdir)/MQTTVersion.c $(srcdir)/MQTTAsync.h ${MQTTLIB_A_TARGET} $(MQTTLIB_CS_TARGET)
	${CC} ${FLAGS_EXE} -o $@ -l${MQTTLIB_A} $(srcdir)/MQTTVersion.c -ldl

strip_options:
	$(eval INSTALL_OPTS := -s)

install-strip: build strip_options install

install: build
	$(INSTALL_DATA) ${INSTALL_OPTS} ${MQTTLIB_C_TARGET} $(DESTDIR)${libdir}
	$(INSTALL_DATA) ${INSTALL_OPTS} ${MQTTLIB_CS_TARGET} $(DESTDIR)${libdir}
	$(INSTALL_DATA) ${INSTALL_OPTS} ${MQTTLIB_A_TARGET} $(DESTDIR)${libdir}
	$(INSTALL_DATA) ${INSTALL_OPTS} ${MQTTLIB_AS_TARGET} $(DESTDIR)${libdir}
	$(INSTALL_PROGRAM) ${INSTALL_OPTS} ${MQTTVERSION_TARGET} $(DESTDIR)${bindir}
	$(LDCONFIG) $(DESTDIR)${libdir}
	ln -s lib$(MQTTLIB_C).so.${MAJOR_VERSION} $(DESTDIR)${libdir}/lib$(MQTTLIB_C).so
	ln -s lib$(MQTTLIB_CS).so.${MAJOR_VERSION} $(DESTDIR)${libdir}/lib$(MQTTLIB_CS).so
	ln -s lib$(MQTTLIB_A).so.${MAJOR_VERSION} $(DESTDIR)${libdir}/lib$(MQTTLIB_A).so
	ln -s lib$(MQTTLIB_AS).so.${MAJOR_VERSION} $(DESTDIR)${libdir}/lib$(MQTTLIB_AS).so
	$(INSTALL_DATA) ${srcdir}/MQTTAsync.h $(DESTDIR)${includedir}
	$(INSTALL_DATA) ${srcdir}/MQTTClient.h $(DESTDIR)${includedir}
	$(INSTALL_DATA) ${srcdir}/MQTTClientPersistence.h $(DESTDIR)${includedir}

uninstall:
	rm $(DESTDIR)${libdir}/lib$(MQTTLIB_C).so.${VERSION}
	rm $(DESTDIR)${libdir}/lib$(MQTTLIB_CS).so.${VERSION}
	rm $(DESTDIR)${libdir}/lib$(MQTTLIB_A).so.${VERSION}
	rm $(DESTDIR)${libdir}/lib$(MQTTLIB_AS).so.${VERSION}
	rm $(DESTDIR)${bindir}/MQTTVersion
	$(LDCONFIG) $(DESTDIR)${libdir}
	rm $(DESTDIR)${libdir}/lib$(MQTTLIB_C).so
	rm $(DESTDIR)${libdir}/lib$(MQTTLIB_CS).so
	rm $(DESTDIR)${libdir}/lib$(MQTTLIB_A).so
	rm $(DESTDIR)${libdir}/lib$(MQTTLIB_AS).so
	rm $(DESTDIR)${includedir}/MQTTAsync.h
	rm $(DESTDIR)${includedir}/MQTTClient.h
	rm $(DESTDIR)${includedir}/MQTTClientPersistence.h

REGEX_DOXYGEN := \
    's;@PROJECT_SOURCE_DIR@/src/\?;;' \
    's;@PROJECT_SOURCE_DIR@;..;' \
    's;@CMAKE_CURRENT_BINARY_DIR@;../build/output;'
SED_DOXYGEN := $(foreach sed_exp,$(REGEX_DOXYGEN),-e $(sed_exp))
define process_doxygen
	cd ${srcdir}; sed $(SED_DOXYGEN) ../doc/${1}.in > ../$(docdir_work)/${1}
	cd ${srcdir}; $(DOXYGEN_COMMAND) ../$(docdir_work)/${1}
endef
html:
	-mkdir -p $(docdir_work)
	-mkdir -p ${docdir}
	$(call process_doxygen,DoxyfileV3ClientAPI)
	$(call process_doxygen,DoxyfileV3AsyncAPI)
	$(call process_doxygen,DoxyfileV3ClientInternal)


clean: debug-clean
		rm -rf ${blddir}/*


clean-binary: debug-clean-binary

debug:
	$(MAKE) -f $(MAKEFILE_BASE).Debug

debug-clean: clean
	$(MAKE) -f $(MAKEFILE_BASE).Debug clean

debug-clean-binary:
	$(MAKE) -f $(MAKEFILE_BASE).Debug clean-binary

release:
	$(MAKE) -f $(MAKEFILE_BASE).Release

release-clean:
	$(MAKE) -f $(MAKEFILE_BASE).Release clean
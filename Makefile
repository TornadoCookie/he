# Generated using Helium v2.0.0 (https://github.com/tornadocookie/he)

PLATFORM?=linux64
DISTDIR?=build

.PHONY: all

ifeq ($(PLATFORM), linux64)
EXEC_EXTENSION=
LIB_EXTENSION=.so
CC=gcc
CFLAGS+=-O2
CFLAGS+=-D RELEASE
CFLAGS+=-D EXEC_EXTENSION=\"\"
CFLAGS+=-D LIB_EXTENSION=\".so\"
endif

ifeq ($(PLATFORM), linux64-debug)
EXEC_EXTENSION=-debug
LIB_EXTENSION=-debug.so
CC=gcc
CFLAGS+=-g
CFLAGS+=-D DEBUG
CFLAGS+=-D EXEC_EXTENSION=\"-debug\"
CFLAGS+=-D LIB_EXTENSION=\"-debug.so\"
endif

ifeq ($(PLATFORM), win64)
EXEC_EXTENSION=.exe
LIB_EXTENSION=.dll
CC=x86_64-w64-mingw32-gcc
CFLAGS+=-O2
CFLAGS+=-D RELEASE
CFLAGS+=-D EXEC_EXTENSION=\".exe\"
CFLAGS+=-D LIB_EXTENSION=\".dll\"
endif

ifeq ($(PLATFORM), win32)
EXEC_EXTENSION=.exe
LIB_EXTENSION=.dll
CC=i686-w64-mingw32-gcc
CFLAGS+=-O2
CFLAGS+=-D RELEASE
CFLAGS+=-D EXEC_EXTENSION=\".exe\"
CFLAGS+=-D LIB_EXTENSION=\".dll\"
endif

PROGRAMS=he
LIBRARIES=

all: $(DISTDIR) $(DISTDIR)/src $(foreach prog, $(PROGRAMS), $(DISTDIR)/$(prog)$(EXEC_EXTENSION)) $(foreach lib, $(LIBRARIES), $(DISTDIR)/$(lib)$(LIB_EXTENSION))
$(DISTDIR)/src:
	mkdir -p $@

$(DISTDIR):
	mkdir -p $@

CFLAGS+=-Isrc
CFLAGS+=-Iinclude
CFLAGS+=-D PLATFORM=\"$(PLATFORM)\"
CFLAGS+=-Wno-unused-result

he_SOURCES+=$(DISTDIR)/src/main.o

$(DISTDIR)/he$(EXEC_EXTENSION): $(he_SOURCES)
	$(CC) -o $@ $^ $(LDFLAGS)

$(DISTDIR)/%.o: %.c
	$(CC) -c $^ $(CFLAGS) -o $@

clean:
	rm -f $(DISTDIR)/src/main.o
	rm -f $(DISTDIR)/he$(EXEC_EXTENSION)

all_dist:
	DISTDIR=$(DISTDIR)/dist/linux64 PLATFORM=linux64 $(MAKE)
	DISTDIR=$(DISTDIR)/dist/linux64-debug PLATFORM=linux64-debug $(MAKE)
	DISTDIR=$(DISTDIR)/dist/win64 PLATFORM=win64 $(MAKE)
	DISTDIR=$(DISTDIR)/dist/win32 PLATFORM=win32 $(MAKE)

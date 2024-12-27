.POSIX:
.SUFFIXES:
.SUFFIXES: .obj .c .res .rc

PROG = TbConf.exe

CC = cc
RM = rm -vf
WINDRES = windres
STRIP = strip

MY_CFLAGS = -Wall -Wextra -Wpedantic -municode $(CFLAGS)
MY_CPPFLAGS = -D_WINDOWS -DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00\
	-DUNICODE -D_UNICODE $(CPPFLAGS)

MY_LDFLAGS = -s\
	-nostdlib -Wl,-e__main -Wl,--enable-stdcall-fixup\
	-ladvapi32 -lcomctl32 -lkernel32 -lshell32 -luser32\
	-Wl,-subsystem,windows:6.2 $(LDFLAGS)

OBJ =\
	src/main.obj\
	src/mincrt.obj\
	src/util.obj\
	src/wndtb.obj\
	src/wndadv.obj\

RES = res/app.res

all: $(PROG)

$(PROG): $(OBJ) $(RES)
	$(CC) -o $@ $(OBJ) $(RES) $(MY_LDFLAGS)
	$(STRIP) $@

.c.obj:
	$(CC) -c $(MY_CPPFLAGS) $(MY_CFLAGS) -o $@ $<

.rc.res:
	$(WINDRES) $(MY_CPPFLAGS) $< -O coff -I. -o $@

clean: cleanres
	$(RM) $(PROG)
	$(RM) $(OBJ)

cleanres:
	$(RM) $(RES)

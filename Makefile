.POSIX:
.SUFFIXES:
.SUFFIXES: .obj .c .res .rc

PROG = TbConf.exe

CC = cc
RM = rm -vf

MY_CFLAGS = -Wall -Wextra -Wpedantic -municode $(CFLAGS)
MY_CPPFLAGS = -D_WINDOWS -DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00\
	-DUNICODE -D_UNICODE $(CPPFLAGS)

MY_LDFLAGS = -s\
	-nostdlib -Wl,-e__main -Wl,--enable-stdcall-fixup\
	-lAdvapi32 -lComCtl32 -lKernel32 -lShell32 -lUser32\
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

.c.obj:
	$(CC) -c $(MY_CPPFLAGS) $(MY_CFLAGS) -o $@ $<

.rc.res:
	windres $(MY_CPPFLAGS) $< -O coff -I. -o $@

clean: cleanres
	$(RM) $(PROG)
	$(RM) $(OBJ)

cleanres:
	$(RM) $(RES)

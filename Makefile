.POSIX:
.SUFFIXES:
.SUFFIXES: .obj .c .res .rc

PROG = TbConf.exe

CC = cc
RM = rm -vf

MY_CFLAGS = -Wall -Wextra -Wpedantic -municode
MY_CPPFLAGS = -D_WINDOWS -DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00 \
	-DUNICODE -D_UNICODE -DNDEBUG

MY_LDFLAGS = -s \
	-nostdlib -Wl,-e__main -Wl,--enable-stdcall-fixup \
	-lAdvapi32 -lComDlg32 -lGdi32 -lKernel32 -lShell32 -lUser32 \
	-Wl,-subsystem,windows:6.2

OBJ = \
	src/main.obj \
	src/mincrt.obj \
	src/wndtb.obj \
	src/colorbtn.obj \
	src/dwmutil.obj \

RES = res/app.res

all: $(PROG)

$(PROG): $(OBJ) $(RES)
	$(CC) -o $@ $(OBJ) $(RES) $(MY_LDFLAGS) $(LDFLAGS)

.c.obj:
	$(CC) -c $(MY_CPPFLAGS) $(CPPFLAGS) $(MY_CFLAGS) $(CFLAGS) -o $@ $<

.rc.res:
	windres $(MY_CPPFLAGS) $(CPPFLAGS) $< -O coff -I. -o $@

clean:
	$(RM) $(PROG)
	$(RM) $(OBJ)
	$(RM) $(RES)

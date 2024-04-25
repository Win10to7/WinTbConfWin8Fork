#!/bin/sh

function _make()
{
	winver="$1"
	exesuffix="$2"
	shift 2

	mingw32-make \
		CPPFLAGS="-DNDEBUG -DWINVER=$winver $CPPFLAGS" \
		CFLAGS="-O2 $CFLAGS" $@

	[ -f TbConf.exe ] && mv TbConf.exe TbConf$exesuffix.exe
	mingw32-make clean
}

mingw32-make clean

_make '0x0A00' '10x86' $@

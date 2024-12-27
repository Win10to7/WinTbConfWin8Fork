#!/bin/sh

if [ -z "$MAKE" ] ; then
	MAKE=ming32-make
	command -v $MAKE 2&>/dev/null || MAKE=make
fi

function _make()
{
	winver="$1"
	exesuffix="$2"
	shift 2

	$MAKE \
		CPPFLAGS="-DNDEBUG -DWINVER=$winver $CPPFLAGS" \
		CFLAGS="-O2 $CFLAGS" $@

	[ -f TbConf.exe ] && mv TbConf.exe TbConf$exesuffix.exe

	$MAKE clean
}

$MAKE clean

_make '0x0A00' '10x86' $@

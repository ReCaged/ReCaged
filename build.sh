#!/bin/sh
#
# RCX - a Free Software, Futuristic, Racing Simulator
#
# Copyright (C) 2012, 2014, 2015 Mats Wahlberg
#
# This file is part of RCX
#
# RCX is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RCX is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with RCX  If not, see <http://www.gnu.org/licenses/>.
#

#
# This is a hackish script for more obscure building processes. Right now it
# only exists for windows builds, to help installing dependencies and building
# an installer. It will print a usage summary when run.
#
# It will be extended for android builds in future versions
#

#native or cross compiling?
case "$(uname -s)" in
	MINGW32*|MSYS*) #mingw/msys
		echo '(detected native build, mingw/msys)'
		BUILDTYPE="W32NATIVE"
		;;
	*)
		echo '(detected cross-compilation build)'
		BUILDTYPE="W32CROSS"
		echo "WARNING: CROSS COMPILING NOT IMPLEMENTED YET!"
		exit 1
		;;
esac


#paths:
#this would be ideal:
#BASEDIR="$HOME/RCX"
#but msys can create home dirs with spaces (depending on user name)...
case $HOME in
	*\ *) #space
		echo '(detected space in $HOME, using /opt/rcx instead of ~/.rcxdev)'
		BASEDIR="/opt/rcxdev"
		;;
	*)
		BASEDIR="$HOME/.rcxdev"
		;;
esac

#just to separate from text above
echo ""

#the actual dirs, based on the one above:
BUILDDIR="$BASEDIR/BUILD"
LIBDIR="$BASEDIR/LIBS"

#create directories, make sure build is empty
rm -rf "$BUILDDIR"
mkdir -p "$LIBDIR" "$BUILDDIR" || exit 1

#and extend custom execution paths
export PATH="$PATH:$LIBDIR/bin"
export CPPFLAGS="-I$LIBDIR/include"
export LDFLAGS="-L$LIBDIR/lib"


#check for what to do
case $1 in
	w32inst)

	echo ""
	echo "Creating w32 installer..."
	echo ""

	#force create autoconf script/files
	echo "Creating autoconf script"
	autoreconf -fvi || exit 1

	#try building
	echo "Running configure and make"
	if ! ( ./configure --enable-w32static --enable-w32console --prefix="$BUILDDIR"  && make clean && make && make install)
	then
		echo ""
		echo "ERROR!"
		echo ""
		exit 1
	fi

	echo ""
	echo "Final tweaks..."
	echo ""

	#install dist files and remove debug symbols in tmp
	make install-strip || exit 1

	#pack
	echo ""
	echo "Building installer..."
	echo ""

	#translate (add CRLF)
	#note: unix2dos got the "-n" option, but most mingw/msys installs got old versions:

	#these aren't installed automatically, copy:
	for f in AUTHORS README COPYING NEWS ChangeLog; do
		cp $f "$BUILDDIR"/$f.txt
		unix2dos "$BUILDDIR"/$f.txt
	done

	#long license texts (all READMEs handled in next line):
	#directory with long license texts:
	cp -r licenses "$BUILDDIR"/licenses
	for f in "$BUILDDIR"/licenses/*; do
		mv "$f" "$f".txt
		unix2dos "$f".txt
	done

	#convert rest of the copyright info (files moved by install)
	#info together with files:
	find "$BUILDDIR" -name "README" -exec mv "{}" "{}".txt \; #add txt suffix
	find "$BUILDDIR" -name "README.txt" -exec unix2dos "{}" \; #add CRLF

	#copy the rest
	cp w32/header.bmp "$BUILDDIR"
	cp w32/installer.nsi "$BUILDDIR"
	cp w32/side.bmp "$BUILDDIR"
	cp w32/version.nsh "$BUILDDIR"

	#move to place
	mv "$BUILDDIR"/bin/rcx "$BUILDDIR"/RCX.exe
	mv "$BUILDDIR"/etc/xdg/rcx "$BUILDDIR"/config
	mv "$BUILDDIR"/share/rcx "$BUILDDIR"/data

	#find nsis the stupid way
	MAKENSIS="$PROGRAMFILES/NSIS/makensis"
	if [ -e "$MAKENSIS" ]
	then
		"$MAKENSIS" "$BUILDDIR"/installer.nsi

		if [ -e "$BUILDDIR"/RCX*Setup.exe ]
		then
			mv "$BUILDDIR"/RCX*Setup.exe .

			echo ""
			echo "Installer should now have been created!"
			echo ""
		else
			echo ""
			echo "ERROR!"
			echo ""
			exit 1
		fi
	else
		echo ""
		echo "Please install NSIS (in the default path!)..."
		echo ""
		exit 1
	fi

	;;


w32deps)

	echo ""
	echo "Getting build dependencies..."
	echo ""

	if [ "$BUILDTYPE" = "W32NATIVE" ]; then
		if ! test -e /etc/fstab
		then
			echo ""
			echo "WARNING: no fstab, /mingw probably not set up! creating!"
			echo ""
			echo "$HOMEDRIVE/mingw /mingw" > /etc/fstab
		fi

		echo "Installing packages using msys-get..."

		#using mingw pre-built packages:
		mingw-get install msys-wget mingw32-gcc mingw32-gcc-g++ mingw32-make mingw32-bzip2 mingw32-libz mingw32-autoconf mingw32-automake msys-vim

		echo "NOTE: WARNINGS ABOVE CAN BE IGNORED! MOST LIKELY THE PACKAGES ARE ALREADY INSTALLED!"

		#yes, I like vim...
		if ! test -e "$HOME/.vimrc"
		then
			echo "...vim text editor not configured. using example config"
			cp /usr/share/vim/vim*/vimrc_example.vim "$HOME/.vimrc"
		fi
	fi

	echo "Compiling and installing libraries..."
	#sdl
	if ! command -v sdl-config 1>/dev/null
	then
		echo ""
		echo "Getting SDL..."
		echo ""
		echo "Figuring out latest version (of 1.2)..."
		SDLV=$(wget 'http://libsdl.org/download-1.2.php' -O - 2>/dev/null|grep "release/SDL-1\.2.*tar.gz\""|cut -d'"' -f2)
		echo "Latest version might be: "$SDLV" - trying..."

		cd "$BUILDDIR"
		wget "http://www.libsdl.org/$SDLV"
		tar xf SDL-* &>/dev/null #ignore gid_t warning

		if ! (cd SDL-*&& \
			./configure --disable-stdio-redirect --prefix="$LIBDIR"&& \
			make install)
		then
			echo ""
			echo "ERROR!"
			echo ""
			exit 1
		fi
	fi

	#ode
	if ! command -v ode-config 1>/dev/null
	then
		echo ""
		echo "Getting ODE..."
		echo ""

		cd "$BUILDDIR"
		wget 'http://sourceforge.net/projects/opende/files/latest/download?source=files'
		tar xf ode-*

		if ! (cd ode-*&& \
			./configure --enable-libccd --prefix="$LIBDIR"&& \
			make install)
		then
			echo ""
			echo "ERROR!"
			echo ""
			exit 1
		fi
	fi

	#png
	if [ ! -e "$LIBDIR/include/png.h" ]
	then
		echo "Getting LIBPNG..."
		echo ""

		echo "Figuring out latest version..."
		PNGV=$(wget 'http://www.libpng.org/pub/png/libpng.html' -O - 2>/dev/null|grep -m 1 "http://prdownloads.*libpng-.*.tar.xz"|cut -d'"' -f2)
		echo "Latest version might be: "$PNGV" - trying..."

		cd "$BUILDDIR"
		wget "$PNGV"
		tar xf libpng* &>/dev/null #ignore gid_t warning

		if ! (cd libpng-*&& \
			./configure --prefix="$LIBDIR"&& \
			make install)
		then
			echo ""
			echo "ERROR!"
			echo ""
			exit 1
		fi
	fi

	#jpeg
	if [ ! -e "$LIBDIR/include/jpeglib.h" ]
	then
		echo ""
		echo "Getting LIBJPEG..."
		echo ""

		echo "Figuring out latest version..."
		JPEGV=$(wget 'http://www.ijg.org/' -O - 2>/dev/null|grep "files/jpegsrc.*tar.gz\""|cut -d'"' -f2)
		echo "Latest version might be: "$JPEGV" - trying..."

		cd "$BUILDDIR"
		wget "http://www.ijg.org/$JPEGV"
		tar xf jpegsrc* &>/dev/null #ignore gid_t warning

		if ! (cd jpeg-*&& \
			./configure --prefix="$LIBDIR"&& \
			make install)
		then
			echo ""
			echo "ERROR!"
			echo ""
			exit 1
		fi
	fi

	#glew
	if [ ! -e "$LIBDIR/include/GL/glew.h" ]
	then
		echo ""
		echo "Getting GLEW..."
		echo ""

		cd "$BUILDDIR"
		wget 'http://sourceforge.net/projects/glew/files/latest/download?source=files'
		tar xf glew-*

		if ! (cd glew-*&& \
			GLEW_DEST="$LIBDIR" make install)
		then
			echo ""
			echo "ERROR!"
			echo ""
			exit 1
		fi
	fi


	#lua TODO!

	if [ "$BUILDTYPE" = "W32NATIVE" ]; then
		#nsis (always try this, best way to update I guess)
		cd "$HOME" #otherwise browser will prevent deletion of BUILDDIR
		echo ""
		echo "Almost done! Now just install NSIS..."
		echo ""
		echo "NOTE: Keep installation path and options/plug-ins at default!"
		cmd //c start "" 'http://sourceforge.net/projects/nsis/files/latest/download?source=files'
	fi

	;;


w32update)

	echo ""
	echo "Getting updates..."
	echo ""

	if [ "$BUILDTYPE" = "W32NATIVE" ]; then
		mingw-get update
		mingw-get upgrade
	fi

	echo ""
	echo "Deleting \"$LIBDIR\" to perform reinstallation"
	echo ""

	rm -rf "$LIBDIR"

	echo "Installing dependencies again"
	"$0" w32deps

	;;


w32quick)

	echo ""
	echo "Doing a quick configuration for manual builds"
	echo ""

	echo "autoreconf..."
	autoreconf -fvi || exit 1

	echo "./configure..."
	./configure --enable-w32static --enable-w32console || exit 1

	echo ""
	echo "Okay! Now just type \"make\" to compile! (and \"src/rcx\" to run)"
	;;


*)

	echo "Usage: \"$0 COMMAND\" where COMMAND is one of the following:"
	echo "	w32inst		- compile and create w32 installer"
	echo "	w32deps		- install everything needed for compiling+packing (+vim)"
	echo "	w32update	- update everything (will delete \"$LIBDIR\")"
	echo "	w32quick	- configure for manual+repeated builds (for developers)"
	echo "(see README for more details)"

	;;

esac


cd "$HOME"
rm -rf "$BUILDDIR"


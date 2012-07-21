#!/bin/sh
#
# ReCaged - a Free Software, Futuristic, Racing Simulator
#
# Copyright (C) 2012 Mats Wahlberg
#
# This file is part of ReCaged.
#
# ReCaged is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# ReCaged is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with ReCaged.  If not, see <http://www.gnu.org/licenses/>.
#

#
# This is a hackish script for installing stuff needed/suggested for w32
# development. It will print a usage summary when run.
#


#start shell, creates home, run script...
#recaged: sdl, ode, glew static, -mconsole


mkdir /tmp/make_w32_tmp &>/dev/null


if [ "$1" = "installer" ]
then
	echo ""
	echo "Creating w32 installer..."
	echo ""

	echo ""
	echo "Compiling..."
	echo ""

	export CPPFLAGS=-I/usr/local/include
	export LDFLAGS=-L/usr/local/lib

	./configure --enable-w32static --enable-w32console && make

	echo ""
	echo "Final tweaks..."
	echo ""

	#remove debug symbols (or: make install-strip?)
	strip recaged.exe

	#translate (add CRLF)
	unix2dos README Readme.txt

	#pack
	echo ""
	echo "Building installer..."
	echo ""

	makensis = "$PROGRAMFILES/NSIS/makensis"
	if [ $makensis ]
	then
		$makensis w32_installer.nsi
	else
		echo ""
		echo "Please install NSIS..."
		echo ""
	fi


elif [ "$1" = "dependencies" ]
then
	echo ""
	echo "Getting minimum dependencies..."
	echo ""

	#using mingw pre-built packages:
	mingw-get install msys-wget mingw32-gcc mingw32-gcc-g++ mingw32-make mingw32-bzip2

	#sdl
	if ! command -v sdl-config &>/dev/null
	then
		echo ""
		echo "Getting SDL..."
		echo ""

		cd /tmp/make_w32_tmp
		wget 'http://www.libsdl.org/tmp/SDL-1.2.tar.gz'
		tar xf SDL-* &>/dev/null #ignore gid_t warning

		if ! (cd SDL-*&& 
			./configure --disable-stdio-redirect --prefix=/usr/local&&
			make install)
		then
			echo ""
			echo "ERROR!"
			echo ""
		fi
	fi

	#ode
	if ! command -v ode-config &>/dev/null
	then
		echo ""
		echo "Getting ODE..."
		echo ""

		cd /tmp/make_w32_tmp
		wget 'http://sourceforge.net/projects/opende/files/latest/download?source=files'
		tar xf ode-*

		if ! (cd ode-*&&
			./configure --enable-libccd --prefix=/usr/local&&
			make install)
		then
			echo ""
			echo "ERROR!"
			echo ""
		fi
	fi

	#glew
	if [ ! -e "/usr/local/include/GL/glew.h" ]
	then
		echo ""
		echo "Getting GLEW..."
		echo ""

		cd /tmp/make_w32_tmp
		wget 'http://sourceforge.net/projects/glew/files/latest/download?source=files'
		tar xf glew-*

		if ! (cd glew-*&& 
			GLEW_DEST=/usr/local make install)
		then
			echo ""
			echo "ERROR!"
			echo ""
		fi
	fi


	#lua TODO!
	#test -e
	#cd /tmp/make_w32_tmp#wget
	#make install

	#nsis
	if [ ! -e "$PROGRAMFILES/NSIS/makensis" ]
	then
		cd /tmp/make_w32_tmp
		wget 'http://prdownloads.sourceforge.net/nsis/nsis-2.46-setup.exe?download'
		echo ""
		echo "Almost done! Now just install NSIS..."
		echo ""
		cmd //c /tmp/make_w32_tmp/nsis*exe
	fi

	echo ""
	echo "That should be it! Check if you got sdl, ode, glew, nsis, etc..."
	echo ""



elif [ "$1" = "extras" ]
then
	#make sure got normal deps (try for missing wget):
	command -v wget &>/dev/null||$0 dependencies

	echo ""
	echo "Getting extra suggestions..."
	echo ""

	#autotools + vim through pre-built:
	mingw-get install mingw32-autoconf mingw32-automake msys-vim

	#if no vim config, default is great:
	test -e "$HOME/.vimrc"||cp /share/vim/vim*/vimrc_example.vim .vimrc


	#compile git:
	if ! command -v git &>/dev/null
	then
		echo ""
		echo "Getting GIT..."
		echo ""

		cd /tmp/make_w32_tmp
		mingw-get install libz mingw32-libiconv&&
		wget 'http://codemonkey.org.uk/projects/git-snapshots/git/git-latest.tar.gz'&&
		tar xzf git-latest.tar.gz
		
		if ! (cd git-*&& 
			make -e CC=gcc NO_OPENSSL=true NO_TCLTK=true NO_GETTEXT=true INSTALL=/bin/install prefix=/usr/local install)
		then
			echo ""
			echo "ERROR!"
			echo ""
		fi
	fi



	echo ""
	echo "That should be it! Check if you got autoconf, automake, vim, git, etc..."
	echo ""



elif [ "$1" = "update" ]
then
	echo ""
	echo "Getting updates..."
	echo ""

	mingw-get update

	echo ""
	echo "TODO: Not updating compiled programs+libraries yet!"
	echo "Please clear out your \"/usr/local\" and rerun this script to get dependencies/extras updated"
	echo ""



else
	echo "Usage: \"./make_w32 command\" where command is one of the following:"
	echo "	* installer	- compile and create w32 installer"
	echo "	* dependencies	- install everything needed for compiling+packing"
	echo "	* extras	- install other stuff (autoconf, automake, git, vim)"
	echo "	* update	- update everything"
	#echo "	* crossinstaller	- cross-compile w32 installer"
	echo "(see README for more details)"
fi

cd /
rm -rf /tmp/make_w32_tmp

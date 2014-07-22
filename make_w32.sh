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



if [ "$1" = "installer" ]
then
	mkdir /tmp/make_w32_build &>/dev/null
	echo ""
	echo "Creating w32 installer..."
	echo ""

	echo ""
	echo "Compiling..."
	echo ""

	export CPPFLAGS="-I/usr/local/include"
	export LDFLAGS="-L/usr/local/lib"

	#try building
	if ! ( ./configure --enable-w32static --enable-w32console --prefix=/tmp/make_w32_build \
		&& make install)
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
	make install-strip

	#pack
	echo ""
	echo "Building installer..."
	echo ""

	#translate (add CRLF)
	#note: unix2dos got the "-n" option, but most mingw/msys installs got old versions:
	#these two aren't installed automatically, copy:
	cp README /tmp/make_w32_build/README.txt
	cp COPYING /tmp/make_w32_build/COPYING.txt
	#add carriage return to gpl text (all readmes handled in next line):
	unix2dos /tmp/make_w32_build/COPYING.txt
	#convert rest of the copyright info (files moved by install)
	find /tmp/make_w32_build -name "README" -exec mv "{}" "{}".txt \; #add txt suffix
	find /tmp/make_w32_build -name "README.txt" -exec unix2dos "{}" \; #add CRLF

	#copy the rest
	cp w32/header.bmp /tmp/make_w32_build
	cp w32/installer.nsi /tmp/make_w32_build
	cp w32/side.bmp /tmp/make_w32_build
	cp w32/version.nsh /tmp/make_w32_build

	#move to place
	mv /tmp/make_w32_build/bin/recaged.exe /tmp/make_w32_build/ReCaged.exe
	mv /tmp/make_w32_build/etc/xdg/recaged /tmp/make_w32_build/config
	mv /tmp/make_w32_build/share/recaged /tmp/make_w32_build/data

	#find nsis the stupid way
	makensis="$PROGRAMFILES/NSIS/makensis"
	if [ -e "$makensis" ]
	then
		"$makensis" /tmp/make_w32_build/installer.nsi

		if [ -e /tmp/make_w32_build/ReCaged*Setup.exe ]
		then
			mv /tmp/make_w32_build/ReCaged*Setup.exe .
			rm -rf /tmp/make_w32_build

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
		echo "Please install NSIS..."
		echo ""
		exit 1
	fi



elif [ "$1" = "dependencies" ]
then
	mkdir /tmp/make_w32_deps &>/dev/null
	echo ""
	echo "Getting build dependencies..."
	echo ""

	#using mingw pre-built packages:
	mingw-get install msys-wget mingw32-gcc mingw32-gcc-g++ mingw32-make mingw32-bzip2

	#sdl
	if ! command -v sdl-config 1>/dev/null
	then
		echo ""
		echo "Getting SDL..."
		echo ""

		cd /tmp/make_w32_deps || exit 1
		wget 'http://www.libsdl.org/tmp/SDL-1.2.tar.gz'
		tar xf SDL-* &>/dev/null #ignore gid_t warning

		if ! (cd SDL-*&& \
			./configure --disable-stdio-redirect --prefix=/usr/local&& \
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

		cd /tmp/make_w32_deps || exit 1
		wget 'http://sourceforge.net/projects/opende/files/latest/download?source=files'
		tar xf ode-*

		if ! (cd ode-*&& \
			./configure --enable-libccd --prefix=/usr/local&& \
			make install)
		then
			echo ""
			echo "ERROR!"
			echo ""
			exit 1
		fi
	fi

	#glew
	if [ ! -e "/usr/local/include/GL/glew.h" ]
	then
		echo ""
		echo "Getting GLEW..."
		echo ""

		cd /tmp/make_w32_deps || exit 1
		wget 'http://sourceforge.net/projects/glew/files/latest/download?source=files'
		tar xf glew-*

		if ! (cd glew-*&& \
			GLEW_DEST=/usr/local make install)
		then
			echo ""
			echo "ERROR!"
			echo ""
			exit 1
		fi
	fi


	#lua TODO!
	#test -e
	#cd /tmp/make_w32_deps#wget
	#make install

	#nsis
	if [ ! -e "$PROGRAMFILES/NSIS/makensis" ]
	then
		cd /tmp/make_w32_deps || exit 1
		wget 'http://prdownloads.sourceforge.net/nsis/nsis-2.46-setup.exe?download'
		echo ""
		echo "Almost done! Now just install NSIS (keep interfaces and plug-ins selected)..."
		echo ""
		cmd //c /tmp/make_w32_deps/nsis*exe
	fi

	cd /
	rm -rf /tmp/make_w32_deps

	echo ""
	echo "That should be it! Check if you got sdl, ode, glew, nsis, etc..."
	echo ""



elif [ "$1" = "devkit" ]
then
	#make sure got normal deps (try for missing wget):
	if ! command -v wget 1>/dev/null
	then
		echo ""
		echo "Needs normal build dependencies, getting first..."
		echo ""
		$0 dependencies
	fi

	mkdir /tmp/make_w32_dev &>/dev/null
	echo ""
	echo "Getting full devkit suggestions..."
	echo ""

	#autotools + vim through pre-built:
	mingw-get install mingw32-autoconf mingw32-automake msys-vim

	#if no vim config, default is great:
	test -e $HOME/.vimrc||cp /share/vim/vim*/vimrc_example.vim $HOME/.vimrc


	#compile git:
	if ! command -v git 1>/dev/null
	then
		echo ""
		echo "Getting GIT..."
		echo ""

		cd /tmp/make_w32_dev || exit 1
		mingw-get install libz mingw32-libiconv&&
		wget 'http://codemonkey.org.uk/projects/git-snapshots/git/git-latest.tar.gz'&&
		tar xzf git-latest.tar.gz
		
		if ! (cd git-*&& \
			make -e CC=gcc NO_OPENSSL=true NO_TCLTK=true NO_GETTEXT=true INSTALL=/bin/install prefix=/usr/local install)
		then
			echo ""
			echo "ERROR!"
			echo ""
			exit 1
		fi
	fi


	cd /
	rm -rf /tmp/make_w32_dev

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
	echo "Deleting \"/usr/local\" to perform reinstallation"
	echo ""

	if command -v git 1>/dev/null
	then
		rm -rf /usr/local
		echo "Installing dependencies+devkit again"
		$0 dependencies
		$0 devkit
	else
		rm -rf /usr/local
		echo "Installing dependencies again"
		$0 dependencies
	fi

	echo ""
	echo "Please check and update NSIS manually"
	echo ""



elif [ "$1" = "crossinstaller" ]
then
	echo ""
	echo "TODO!"
	echo ""
	exit 1



else
	echo "Usage: \"./make_w32 command\" where command is one of the following:"
	echo "	* installer	- compile and create w32 installer"
	echo "	* dependencies	- install everything needed for compiling+packing"
	echo "	* devkit	- install dependencies+tools (autotools, git, vim)"
	echo "	* update	- update everything (will delete \"/usr/local\")"
	#echo "	* crossinstaller	- cross-compile w32 installer"
	echo "(see README for more details)"
fi


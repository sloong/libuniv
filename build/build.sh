#!/bin/bash
#
#echo '$0: '$0  
#echo "pwd: "`pwd`  
#echo "scriptPath1: "$(cd `dirname $0`; pwd)  
#echo "scriptPath2: "$(dirname $(readlink -f $0))

#WORKFOLDER=`pwd`
#echo "Workfolder: "$WORKFOLDER


show_help(){
	echo -e "build.sh [operation]
operation:
	-r: to build release version 
	-d: to build debug version 
	-rz: build release to tar.gz
	-dz: build debug to tar.gz
	-i: build debug and install"
}


SCRIPTFOLDER=$(dirname $(readlink -f $0))
#echo "ScriptFolder: "$SCRIPTFOLDER
# cd to current file folder
cd $SCRIPTFOLDER

# default value is debug
VERSION_STR=$(cat $SCRIPTFOLDER/../version)
PROJECT=libuniv
MAKEFLAG=debug
CMAKEFLAG=Debug
CMAKE_FILE_PATH=$SCRIPTFOLDER/../univ
OUTPATH=$SCRIPTFOLDER/$MAKEFLAG/v$VERSION_STR

clean(){
	rm -rdf $MAKEFLAG
}

build(){
	if [ ! -d $MAKEFLAG  ];then
		mkdir $MAKEFLAG
	fi
	cd $MAKEFLAG
	cmake -DCMAKE_BUILD_TYPE=$CMAKEFLAG $CMAKE_FILE_PATH
	make
	cd ../
	copy_file
}

copy_file(){
	rm -rdf $OUTPATH
	mkdir -p $OUTPATH
	cp -f $SCRIPTFOLDER/$MAKEFLAG/libuniv.so $OUTPATH/libuniv.so
	mkdir -p $OUTPATH/include/univ/
	cp -f $SCRIPTFOLDER/../univ/*.h* $OUTPATH/include/univ/
	cp -f $SCRIPTFOLDER/install.sh $OUTPATH/install.sh
}

build_debug(){
	MAKEFLAG=debug
	CMAKEFLAG=Debug
	build
}

build_release(){
	MAKEFLAG=release
	CMAKEFLAG=Release
	clean
	build
}

zip(){
	ZIPOUTPUTPATH=$SCRIPTFOLDER/libuniv_v$VERSION_STR.tar.gz
	cd $OUTPATH/
	tar -czf $ZIPOUTPUTPATH ./*
}


install(){
	cd $OUTPATH
	sudo ./install.sh
}

# -eq 等于,
# -ne 不等于
# -gt 大于,
# -ge 大于等于,
# -lt 小于,
# -le 小于等于
if [ $# -eq 0 ]; then
	build
	exit
fi

if [ $# -ge 1 ]; then
	case $1 in 
		-r) build_release;;
		-d) build_debug;;
		-rz) 
			build_release
			zip;;
		-dz) 
			build_debug
			zip;;
		-i) install;;
		* ) show_help;;
	esac
fi

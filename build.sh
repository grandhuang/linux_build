#!/bin/bash

CUR_DIR=$(pwd)
echo $CUR_DIR

option=$1

BUILD_TIME=$(date +"%y%m%d%H%M")

HVER="H0"
SWVER="S00"
INTVER="1.0.0"
RELVER="alpha"
MANUFACTURER="SANHONG"
BUILD_INFO_DIR=buildinfo

build_log(  )
{
    echo ${HVER}${SWVER}
    echo ${INTVER}.${BUILD_TIME}_${RELVER}
    # echo "Last build:$(date)">>${BUILD_INFO_DIR}/buildtime.txt
	# echo "Version:${INTVER}.${BUILD_TIME}_${RELVER}">>${BUILD_INFO_DIR}/buildtime.txt
    echo "build by $(whoami)"
}

help_info(  )
{
   echo "$0 all             --------编译全部"
   echo "$0 clean/clear     --------清除编译生成内容"
   echo "$0 help/h          --------显示帮助内容"
}


case $option in
	all)
        build_log
        make -j4
		;;
	clean|clear)
		make clean
		;;
	h|help)
		help_info $0
		;;
	*)
		echo "Wrong parameter: $@"
		help_info
		exit 1
		;;
esac



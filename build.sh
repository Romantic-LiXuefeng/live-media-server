#!/bin/sh

RED="\\e[31m"
GREEN="\\e[32m"
YELLOW="\\e[33m"
BLACK="\\e[0m"

_3rdParty_dir="3rdparty"
current_dir=`pwd`
library_dir="library"
gcc_version=`gcc -dumpversion`

function build_openssl()
{
	cd $current_dir
    if ! test -f $library_dir/openssl/lib/libcrypto.a; then
        if test -d $_3rdParty_dir/openssl-1.0.2e; then
            rm -rf $_3rdParty_dir/openssl-1.0.2e
        fi
        
		cd $_3rdParty_dir && unzip openssl-1.0.2e.zip && 
		cd openssl-1.0.2e && ./config --prefix=$current_dir/$library_dir/openssl && make && make install
        
		if test $? -ne 0; then
			echo -e $RED"...\t\tFailed!"$BLACK
            exit -1
        fi

		rm -rf $current_dir/$_3rdParty_dir/openssl-1.0.2e
    fi
}

function build_cares()
{
	cd $current_dir
	if ! test -f $library_dir/cares/lib/libcares.a; then
		if test -d $_3rdParty_dir/c-ares-1.12.0; then
            rm -rf $_3rdParty_dir/c-ares-1.12.0
        fi

		cd $_3rdParty_dir && unzip c-ares-1.12.0.zip &&
		cd c-ares-1.12.0 && ./configure --prefix=$current_dir/$library_dir/cares && make && make install

		if test $? -ne 0; then
            echo -e $RED"...\t\tFailed!"$BLACK
            exit -1
        fi

        rm -rf $current_dir/$_3rdParty_dir/c-ares-1.12.0
	fi
}

function build_pcre()
{
	cd $current_dir
	if ! test -f $library_dir/pcre/lib/libpcre.a; then
		if test -d $_3rdParty_dir/pcre-8.40; then
            rm -rf $_3rdParty_dir/pcre-8.40
        fi

		cd $_3rdParty_dir && unzip pcre-8.40.zip &&
		cd pcre-8.40 && ./configure --prefix=$current_dir/$library_dir/pcre && make && make install

		if test $? -ne 0; then
            echo -e $RED"...\t\tFailed!"$BLACK
            exit -1
        fi

        rm -rf $current_dir/$_3rdParty_dir/pcre-8.40
	fi
}

function build_unwind()
{
	cd $current_dir
    if ! test -f $library_dir/unwind/lib/libunwind.a; then
        if test -d $_3rdParty_dir/libunwind-1.0.1; then
            rm -rf $_3rdParty_dir/libunwind-1.0.1
        fi
        
		cd $_3rdParty_dir && unzip libunwind-1.0.1.zip && 
		cd libunwind-1.0.1 && ./configure --prefix=$current_dir/$library_dir/unwind && make && make install
        
		if test $? -ne 0; then
			echo -e $RED"...\t\tFailed!"$BLACK
            exit -1
        fi

		rm -rf $current_dir/$_3rdParty_dir/libunwind-1.0.1
    fi
}

function build_gperftools()
{
	cd $current_dir
    if ! test -f $library_dir/gperftools/lib/libtcmalloc.a; then
        if test -d $_3rdParty_dir/gperftools-2.1; then
            rm -rf $_3rdParty_dir/gperftools-2.1
        fi
        
		cd $_3rdParty_dir && unzip gperftools-2.1.zip && 
		cd gperftools-2.1 && ./configure --prefix=$current_dir/$library_dir/gperftools LDFLAGS=-L$current_dir/$library_dir/unwind/lib CPPFLAGS=-I$current_dir/$library_dir/unwind/include
		make && make install
        
		if test $? -ne 0; then
			echo -e $RED"...\t\tFailed!"$BLACK
            exit -1
        fi

		rm -rf $current_dir/$_3rdParty_dir/gperftools-2.1
    fi
}

function build_http_parser()
{
	cd $current_dir
	if ! test -f $library_dir/http-parser/libhttp_parser.a; then
        if test -d $_3rdParty_dir/http-parser-2.1; then
            rm -rf $_3rdParty_dir/http-parser-2.1
        fi
    
        cd $_3rdParty_dir && unzip http-parser-2.1.zip &&
        cd http-parser-2.1 && make package
		parser_dir=$current_dir/$library_dir/http-parser
		mkdir -p $parser_dir && cp libhttp_parser.a $parser_dir && cp http_parser.h $parser_dir
    	
        if test $? -ne 0; then
            echo -e $RED"...\t\tFailed!"$BLACK
            exit -1
        fi

        rm -rf $current_dir/$_3rdParty_dir/http-parser-2.1
    fi
}

function build_codec()
{
	cd $current_dir
	if ! test -f $library_dir/codec/lib/libcodec.so; then
		if test -d $_3rdParty_dir/codec; then
			rm -rf $_3rdParty_dir/codec
		fi

		cd $_3rdParty_dir && unzip codec.zip && cd codec && mkdir build && cd build
		cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$current_dir/$library_dir/codec
		make && make install

		if test $? -ne 0; then
            echo -e $RED"...\t\tFailed!"$BLACK
            exit -1
        fi

		rm -rf $current_dir/$_3rdParty_dir/codec
	fi
}

function build_zlib()
{
	cd $current_dir
	if ! test -f $library_dir/zlib/lib/libz.so; then
		if test -d $_3rdParty_dir/zlib-1.2.11; then
			rm -rf $_3rdParty_dir/zlib-1.2.11
		fi

		cd $_3rdParty_dir && unzip zlib-1.2.11.zip && cd zlib-1.2.11
		./configure --prefix=$current_dir/$library_dir/zlib
		make && make install

		if test $? -ne 0; then
            echo -e $RED"...\t\tFailed!"$BLACK
            exit -1
        fi

		rm -rf $current_dir/$_3rdParty_dir/zlib-1.2.11
	fi
}

function runCommand()
{
	echo -n -e $YELLOW"run $1"$BLACK
	$*
	if test $? -ne 0;then
		echo -e $RED"\t............................\t\tFailed!"$BLACK
		exit -1	
	fi
	echo -e $GREEN"\t............................\t\tOK"$BLACK
}

function check_dir()
{
	if ! test -d $library_dir; then
		mkdir $library_dir
	fi
}

function check_kernel_version()
{
	base=`uname -r`
	version=`echo ${base%-*}`
	major=`echo | awk '{split("'${version}'", array, ".");print array[1]}'`
	minor=`echo | awk '{split("'${version}'", array, ".");print array[2]}'`
	revision=`echo | awk '{split("'${version}'", array, ".");print array[3]}'`

	if [ $major -lt 2 ]; then
		echo -e $RED"\ncurrent kernel version is $version, kernel version must be >= 2.6.32\n"$BLACK
		return 1
	elif [ $major -eq 2 ]; then
		if [ $minor -lt 6 ]; then
        	echo -e $RED"\ncurrent kernel version is $version, kernel version must be >= 2.6.32\n"$BLACK
        	return 2
    	elif [ $minor -eq 6 ]; then
			if [ $revision -lt 32 ]; then
        		echo -e $RED"\ncurrent kernel version is $version, kernel version must be >= 2.6.32\n"$BLACK
      			return 3
    		fi
		fi
	fi

	return 0
}

runCommand check_kernel_version
runCommand check_dir
runCommand build_openssl
runCommand build_unwind
runCommand build_gperftools
runCommand build_cares
runCommand build_pcre
runCommand build_http_parser
runCommand build_codec
runCommand build_zlib

echo -e $GREEN"build library success."$BLACK

if ! test -d $current_dir/build; then
    mkdir $current_dir/build
fi

cd $current_dir/build
cmake .. -DCMAKE_BUILD_TYPE=Debug 
make

if test $? -ne 0;then
    echo -e $RED"build lms failed.........."$BLACK
else
    echo -e $GREEN"build lms success.........."$BLACK
fi

cd $current_dir

echo -e "You can build:\n\t\"mkdir build && cd build\""
echo -e "build release\t\"cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_GPERF=true -DCMAKE_INSTALL_PREFIX=/usr/local\""
echo -e "build debug\t\"cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_GPERF=true -DCMAKE_INSTALL_PREFIX=/usr/local\""
echo -e "make && make install to build and publish program"

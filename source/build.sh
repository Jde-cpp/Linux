#!/bin/bash
debug=${1:-1}
clean=${2:-0}
echo "Build Drive.Linux"
cd "${0%/*}"
if [ $clean -eq 1 ]; then
	make clean DEBUG=$debug
	if [ $debug -eq 1 ]; then
		ccache g++-8 -c -g -pthread -std=c++17 -Wall -Wno-unknown-pragmas -DJDE_NATIVE_EXPORTS  -fPIC -O0 -fsanitize=address -fno-omit-frame-pointer ./pc.h -o .obj/debug/stdafx.h.gch -I/home/duffyj/code/libraries/json/include -I/home/duffyj/code/libraries/spdlog/include  -I$BOOST_ROOT -I../../Framework/source
	else
		ccache g++-8 -c -g -fPIC -pthread -std=c++17 -Wall -Wno-unknown-pragmas -DJDE_NATIVE_EXPORTS  -march=native -DNDEBUG -O3 ./pc.h -o .obj/release2/stdafx.h.gch -I/home/duffyj/code/libraries/json/include -I/home/duffyj/code/libraries/spdlog/include  -I$BOOST_ROOT -I../../Framework/source
	fi;
	if [ $? -eq 1 ]; then
		exit 1
	fi;
fi
make -j7 DEBUG=$debug
cd -
exit $?
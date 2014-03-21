
all: debug release

release:
	mkdir -p build/release
	g++ -O3 -Wall -fmessage-length=0 -std=c++0x -Ilib/src -o build/release/jpg2bmp sample/src/main.cpp lib/src/*.cpp

debug:
	mkdir -p build/debug
	g++ -DPROJECT_DEBUG_BUILD -O0 -g3 -Wall -fmessage-length=0 -std=c++0x -Ilib/src -o build/debug/jpg2bmp sample/src/main.cpp lib/src/*.cpp

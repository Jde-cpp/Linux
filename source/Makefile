CCACHE_EXISTS := $(shell ccache -V)
ifdef CCACHE_EXISTS
    CC := ccache $(CC)
    CXX := ccache g++-8
endif
DEBUG ?= 1
CXXFLAGS_BASE = -c -g -pthread -std=c++17 -Wall -Wno-unknown-pragmas -DJDE_NATIVE_EXPORTS  -fPIC
BIN_DIR_BASE:=../../bin/
OUT_DIR_BASE := .obj/
# -lboost_system -lmatio
ifeq ($(DEBUG), 1)
	OUT_DIR=$(OUT_DIR_BASE)debug
	CXXFLAGS= $(CXXFLAGS_BASE) -O0 -fsanitize=address -fno-omit-frame-pointer
	BIN_DIR=$(BIN_DIR_BASE)debug
else
	OUT_DIR=$(OUT_DIR_BASE)release
	CXXFLAGS= $(CXXFLAGS_BASE) -march=native -DNDEBUG -O3
	BIN_DIR=$(BIN_DIR_BASE)release
endif

OUTPUT = $(BIN_DIR)/libNative.a

#CXX=g++-8

OBJECTS = $(OUT_DIR)/LinuxDrive.o $(OUT_DIR)/LinuxTimezone.o $(OUT_DIR)/LinuxApp.o
#$(OUT_DIR)/LinuxApp.o
#LDFLAGS = -pthread -shared -Wl,-z,origin -Wl,-rpath='$$ORIGIN'

all: $(OUTPUT)
	$(NOECHO) $(NOOP)

$(OUTPUT): $(OUT_DIR)/stdafx.h.gch $(OBJECTS)
	ar rsv $@ $^

$(OUT_DIR)/stdafx.h.gch: pc.h
	$(CXX) $(CXXFLAGS) ./pc.h -o $(OUT_DIR)/stdafx.h.gch -I/home/duffyj/code/libraries/json/include -I/home/duffyj/code/libraries/spdlog/include  -I$$BOOST_ROOT -I../../Framework/source
######################################################################################
CXXFLAGS2 = $(CXXFLAGS) -include stdafx.h -I$(OUT_DIR)
$(OUT_DIR)/%.o: %.cpp pc.h
	$(CXX) $(CXXFLAGS2) $(INCLUDE) ./$< -o$@

clean:
	rm -rf -d $(OUT_DIR)/*.*
	rm -rf -d $(OUTPUT)
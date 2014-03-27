
CC=g++
CPP_FLAGS=-Wall -fmessage-length=0 -std=c++0x
BUILD_DIR=build

LIB_DIR=lib
EXEC_DIR=sample
TEST_DIR=test

SOURCE_SUBDIR=src

LIB_C_FILES=$(wildcard $(LIB_DIR)/$(SOURCE_SUBDIR)/*.c)
LIB_H_FILES=$(wildcard $(LIB_DIR)/$(SOURCE_SUBDIR)/*.h)
LIB_CPP_FILES=$(wildcard $(LIB_DIR)/$(SOURCE_SUBDIR)/*.cpp)
LIB_HPP_FILES=$(wildcard $(LIB_DIR)/$(SOURCE_SUBDIR)/*.hpp)

LIB_HEADERS=$(LIB_H_FILES) $(LIB_HPP_FILES)
LIB_SOURCES=$(LIB_C_FILES) $(LIB_CPP_FILES)

$(BUILD_DIR)/release/jpg2bmp: $(EXEC_DIR)/$(SOURCE_SUBDIR)/main.cpp $(LIB_HEADERS) $(LIB_SOURCES)
	mkdir -p $(BUILD_DIR)/release
	$(CC) -O3 $(CPP_FLAGS) -I$(LIB_DIR)/$(SOURCE_SUBDIR) -o $@ $(EXEC_DIR)/$(SOURCE_SUBDIR)/main.cpp $(LIB_SOURCES)

release: $(BUILD_DIR)/release/jpg2bmp

$(BUILD_DIR)/debug/jpg2bmp: $(EXEC_DIR)/$(SOURCE_SUBDIR)/main.cpp $(LIB_HEADERS) $(LIB_SOURCES)
	mkdir -p $(BUILD_DIR)/debug
	$(CC) -DPROJECT_DEBUG_BUILD -O0 -g3 $(CPP_FLAGS) -I$(LIB_DIR)/$(SOURCE_SUBDIR) -o $@ $(EXEC_DIR)/$(SOURCE_SUBDIR)/main.cpp $(LIB_SOURCES)

debug: $(BUILD_DIR)/debug/jpg2bmp

$(BUILD_DIR)/test/main: test/src/main.cpp $(LIB_HEADERS) $(LIB_SOURCES)
	mkdir -p $(BUILD_DIR)/test
	$(CC) -DPROJECT_DEBUG_BUILD -O0 -g3 $(CPP_FLAGS) -I$(LIB_DIR)/$(SOURCE_SUBDIR) -o $@ $(TEST_DIR)/$(SOURCE_SUBDIR)/main.cpp $(LIB_SOURCES)

test: $(BUILD_DIR)/test/main

clean:
	rm -rf $(BUILD_DIR)

all: release debug test

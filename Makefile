# options
#
# uncomment to compile boost in statically
# STATIC_BUILD=YES

# discover the liklihood of what version of FUSE we're using
# also set the compiler type; clang if on mac, gcc if on linux
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
    CXX=g++
    FUSE=fuse
else
    CXX=clang++
    FUSE=osxfuse
endif

# standard library search paths
LDFLAGS +=  -L/usr/local/lib -L/usr/lib

# try to find path of where boost is probably installed
ifeq ($(wildcard /usr/local/lib/libboost*),)
    BOOST_PATH= /usr/lib
else
    BOOST_PATH= /usr/local/lib
endif

# try to find where boost headers are likely installed
ifeq ($(wildcard /usr/local/include/boost/%),)
    BOOST_HEADERS= /usr/local/include/boost
else
    BOOST_HEADERS= /usr/include/boost
endif

# prefer dynamic libs for security
ifdef STATIC_BUILD
BOOST_LD= $(BOOST_PATH)/libboost_filesystem.a \
		  $(BOOST_PATH)/libboost_system.a \
		  $(BOOST_PATH)/libboost_program_options.a \
		  $(BOOST_PATH)/libboost_random.a \
		  $(BOOST_PATH)/libboost_regex.a
else
BOOST_LD= $(BOOST_PATH)/libboost_filesystem.so \
          $(BOOST_PATH)/libboost_system.so \
          $(BOOST_PATH)/libboost_program_options.so \
          $(BOOST_PATH)/libboost_random.so \
          $(BOOST_PATH)/libboost_regex.so
endif

# compilation flags
CXXFLAGS_FUSE= -I/usr/local/include/$(FUSE)  -DFUSE_USE_VERSION=26
CXXFLAGS ?= -ggdb \
          -Os \
          -ffast-math \
          -funroll-loops \
          -Wno-ctor-dtor-privacy
CXXFLAGS += -std=c++11 \
			-I$(BOOST_HEADERS) \
			-Iinclude -D_FILE_OFFSET_BITS=64

# specify locations of all source files
SOURCES := $(wildcard src/teasafe/*.cpp)
MAKE_TeaSafe_SRC := $(wildcard src/maketeasafe/*.cpp)
TEST_SRC := $(wildcard src/test/*.cpp)
FUSE_SRC := $(wildcard src/fuse/*.cpp)
CIPHER_SRC := $(wildcard src/cipher/*.cpp)
UTILITY_SRC := $(wildcard src/utility/*.cpp)

# specify object locations; they will be dumped in several directories
# obj, obj-maketeasafe, obj-test, obj-fuse and obj-cipher
OBJECTS := $(addprefix obj/,$(notdir $(SOURCES:.cpp=.o)))
OBJECTS_MAKEBIN := $(addprefix obj-maketeasafe/,$(notdir $(MAKE_TeaSafe_SRC:.cpp=.o)))
OBJECTS_TEST := $(addprefix obj-test/,$(notdir $(TEST_SRC:.cpp=.o)))
OBJECTS_FUSE := $(addprefix obj-fuse/,$(notdir $(FUSE_SRC:.cpp=.o)))
OBJECTS_CIPHER := $(addprefix obj-cipher/,$(notdir $(CIPHER_SRC:.cpp=.o)))
OBJECTS_UTILITY := $(addprefix obj-utility/,$(notdir $(UTILITY_SRC:.cpp=.o)))

# the executable used for running the test harness
TEST_EXECUTABLE=test_$(UNAME)

# the executable used for creating a teasafe image
MAKETeaSafe_EXECUTABLE=maketeasafe_$(UNAME)

# the fuse-layered executable
FUSE_LAYER=teasafe_$(UNAME)

# simple utility programs
SHELL_BIN=teashell_$(UNAME)

# build the different object files
obj/%.o: src/teasafe/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

obj-test/%.o: src/test/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

obj-maketeasafe/%.o: src/maketeasafe/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

obj-utility/%.o: src/utility/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

obj-fuse/%.o: src/fuse/%.cpp
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_FUSE) -c -o $@ $<

obj-cipher/%.o: src/cipher/%.cpp
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_FUSE) -c -o $@ $<

all: $(SOURCES) $(CIPHER_SRC) directoryObj directoryObjCipher \
     $(OBJECTS) $(OBJECTS_CIPHER) libBuilder \
     $(TEST_SRC) $(TEST_EXECUTABLE) $(FUSE_LAYER) $(MAKETeaSafe_EXECUTABLE) \
     $(SHELL_BIN)

lib: $(SOURCES) $(CIPHER_SRC) directoryObj directoryObjCipher $(OBJECTS) $(OBJECTS_CIPHER) libBuilder

$(TEST_EXECUTABLE): directoryObjTest $(OBJECTS_TEST)
	$(CXX) $(LDFLAGS) $(OBJECTS_TEST) ./libteasafe.a $(BOOST_LD) -o $@

$(MAKETeaSafe_EXECUTABLE): directoryObjMakeBfs $(OBJECTS_MAKEBIN)
	$(CXX) $(LDFLAGS) $(OBJECTS_MAKEBIN) ./libteasafe.a $(BOOST_LD) -o $@

$(SHELL_BIN): directoryObjUtility $(OBJECTS_UTILITY)
	$(CXX) $(LDFLAGS) $(OBJECTS_UTILITY) ./libteasafe.a $(BOOST_LD) -o $@

$(FUSE_LAYER): directoryObjFuse $(OBJECTS_FUSE)
	$(CXX) $(LDFLAGS) -l$(FUSE) $(OBJECTS_FUSE) ./libteasafe.a -l$(FUSE) $(BOOST_LD) -o $@

clean:
	/bin/rm -fr obj obj-maketeasafe obj-test obj-fuse test_$(UNAME) maketeasafe_$(UNAME) teasafe_$(UNAME) teashell_$(UNAME) obj-cipher obj-utility libteasafe.a

directoryObj:
	/bin/mkdir -p obj

directoryObjTest:
	/bin/mkdir -p obj-test

directoryObjMakeBfs:
	/bin/mkdir -p obj-maketeasafe

directoryObjFuse:
	/bin/mkdir -p obj-fuse

directoryObjCipher:
	/bin/mkdir -p obj-cipher

directoryObjUtility:
	/bin/mkdir -p obj-utility

libBuilder:
	/usr/bin/ar rcs libteasafe.a obj/* obj-cipher/*

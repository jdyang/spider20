WORKROOT=../../../public/
SELIBPATH=$(WORKROOT)/selib


CPP=g++
AR=ar

vpath %.h ../..
vpath %.cpp ../..

INCLUDE= -I./ \
		  -I$(SELIBPATH)/include \

ifeq "$(MAKECMDGOALS)" "release"
    DEFINES=$(COMMON_DEFINES) -DNDEBUG
    CPPFLAGS= -Wno-deprecated
    CFLAGS= -O2 -Wall $(DEFINES) $(INCLUDE)
    CXXFLAGS= -O2 -Wall $(DEFINES) $(INCLUDE)
else
    ifeq "$(MAKECMDGOALS)" "withpg"
        DEFINES=$(COMMON_DEFINES)
        CPPFLAGS= -Wno-deprecated
        CFLAGS= -g -pg -Wall $(DEFINES) $(INCLUDE)
        CXXFLAGS= -g -pg -Wall $(DEFINES) $(INCLUDE)
    else
        DEFINES=$(COMMON_DEFINES)
        CPPFLAGS= -Wno-deprecated
        CFLAGS= -g -Wall $(DEFINES) $(INCLUDE)
        CXXFLAGS= -g -Wall $(DEFINES) $(INCLUDE)
    endif
endif

COMMON_DEFINES = -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=
1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -DSTDC_HEADERS=1 -DSIZEOF_UNSIGNED_SHORT=2 -DSIZEOF_UNSIGNED_INT=4 -DSIZEOF_UNSIGNED_LONG=8 -D
HAVE_THREADS=1 -DHAVE_GETHOSTBYNAME=1 -DHAVE_SOCKET=1 -DHAVE_INET_ATON=1 -DHAVE_INET_ADDR=1 -DHAVE_INET_PTON=1 -DHAVE_POLL=1 -DHAVE_SELECT=1 -DHAVE_S
TRERROR=1


LIB= -lpthread \
	 -L$(SELIBPATH)/lib \
     -lutil \
	 -L/usr/local/lib -llog4cxx \
	 -L/usr/local/lib -lapr-1\
	 -L/usr/lcoal/lib -laprutil-1\

all : test_spider_statis

OBJS= spider_statis.o \
      test_spider_statis.o

clean :
	rm -rf *.o *~ test_spider_statis
test_spider_statis : $(OBJS)
	$(CPP) $(CPPFLAGS) -o $@ $^ $(LIB)


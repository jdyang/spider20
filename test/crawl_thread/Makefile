SELIB=../../../public/selib
NLP=../../../nlp

CPP=g++
AR=ar

vpath %.h ../.. 
vpath %.cpp ../.. 

INCLUDE= -I./ \
          -I../../ \
          -I$(SELIB)/include \
		  -I$(NLP)/include

COMMON_DEFINES=-DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -DSTDC_HEADERS=1 -DSIZEOF_UNSIGNED_SHORT=2 -DSIZEOF_UNSIGNED_INT=4 -DSIZEOF_UNSIGNED_LONG=8 -DHAVE_THREADS=1 -DHAVE_GETHOSTBYNAME=1 -DHAVE_SOCKET=1 -DHAVE_INET_ATON=1 -DHAVE_INET_ADDR=1 -DHAVE_INET_PTON=1 -DHAVE_POLL=1 -DHAVE_SELECT=1 -DHAVE_STRERROR=1

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




LIB= -L $(SELIB)/lib \
     -L $(NLP)/lib \
	 -ltse \
	 -lcode \
	 -lurl \
	 -lnet \
	 -lutil \
	 -lpthread \
	 -lcrypto

all : test_crawl_thread

OBJS= url_output.o \
      page_output.o \
	  level_pool.o \
	  selected_queue.o \
	  utf8_converter.o \
	  dns_client.o \
      spider.o \
      test_crawl_thread.o

clean :
	rm -rf *.o *~ test_crawl_thread
test_crawl_thread : $(OBJS) 
	$(CPP) $(CPPFLAGS) -o $@ $^ $(LIB)

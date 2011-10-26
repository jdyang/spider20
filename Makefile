WORKROOT=../../../public/
SELIBPATH=$(WORKROOT)/selib
LIBPATH=$(SELIBPATH)/lib
SPIDERLIBPATH=../../../spider/spider_lib
DBLIBPATH=$(WORKROOT)/dblib
NLPLIBPATH=$(WORKROOT)/../nlp

INCLUDE = -I./	\
	      -I$(SELIBPATH)/include \
		  -I$(SPIDERLIBPATH)/include/htmlcxx \
			-I$(SPIDERLIBPATH)/include \
			-I$(DBLIBPATH)/include  \
		  -I$(NLPLIBPATH)/include
		

LIB     = -L./	\
	      -L$(SELIBPATH)/lib \
		  -L$(SPIDERLIBPATH)/lib \
		  -L$(DBLIBPATH)/lib \
		  -L$(NLPLIBPATH)/lib \
	      -lurl \
		  -lextractor \
	      -lnet \
	      -lutil \
		  -ltse \
		  -lfdb \
		  -lcmysql \
	      -L/usr/local/lib -llog4cxx \
	      -L/usr/local/lib -lapr-1\
	      -L/usr/local/lib -laprutil-1\
	      -lpthread \
	      -lz \
		  -lcrypto \
		  -lutil \
		  -lurltools \
		  -lhtml \
		  -lhtmlcxx \
		  -lurl \
		   -lurlpattern \
		  -L/usr/local/lib -lmysqlclient \
		  -lcrypto	\
		  -lcode	\
		  -lboost_regex-gcc41-mt \
		  -lurlpattern \
		  -lextractor

COMMON_DEFINES = -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -DSTDC_HEADERS=1 -DSIZEOF_UNSIGNED_SHORT=2 -DSIZEOF_UNSIGNED_INT=4 -DSIZEOF_UNSIGNED_LONG=8 -DHAVE_THREADS=1 -DHAVE_GETHOSTBYNAME=1 -DHAVE_SOCKET=1 -DHAVE_INET_ATON=1 -DHAVE_INET_ADDR=1 -DHAVE_INET_PTON=1 -DHAVE_POLL=1 -DHAVE_SELECT=1 -DHAVE_STRERROR=1 


ifeq "$(MAKECMDGOALS)" "release"
	DEFINES=$(COMMON_DEFINES) -DNDEBUG
	CPPFLAGS= -Wno-deprecated -g
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


CC  = g++ 
CPP = g++
CXX = g++ 
AR  = ar

#=========================================================================
LIBS=libpredator.a
RELEASE_LIBS= 
EXECUTABLE = predator
TEST_EXEC = predator_test
INSTALL_PATH=./output

LIB_OBJS= predator.o \
          urlpool.o \
	

all	: clean  $(LIBS) $(EXECUTABLE)

release : rebuild
	rm -rf $(INSTALL_PATH)
	mkdir $(INSTALL_PATH)/
	mkdir $(INSTALL_PATH)/bin
	mkdir $(INSTALL_PATH)/conf
	mkdir $(INSTALL_PATH)/log
	mkdir $(INSTALL_PATH)/data
	cp -f $(EXECUTABLE) input.url start_predator.sh $(INSTALL_PATH)/bin
	cp -f *.conf log4cxx.cfg $(INSTALL_PATH)/conf

withpg : rebuild
	rm -rf $(INSTALL_PATH)
	mkdir $(INSTALL_PATH)/
	mkdir $(INSTALL_PATH)/bin
	mkdir $(INSTALL_PATH)/conf
	mkdir $(INSTALL_PATH)/log
	mkdir $(INSTALL_PATH)/data
	cp -f $(EXECUTABLE) $(INSTALL_PATH)/bin
	cp -f *.conf log4cxx.cfg $(INSTALL_PATH)/conf
test: rebuild
	rm -rf $(INSTALL_PATH)
	mkdir $(INSTALL_PATH)/
	mkdir $(INSTALL_PATH)/bin
	mkdir $(INSTALL_PATH)/conf
	mkdir $(INSTALL_PATH)/log
	mkdir $(INSTALL_PATH)/data
	cp -f $(TEST_EXEC) $(INSTALL_PATH)/bin
	cp -f *.conf log4cxx.cfg $(INSTALL_PATH)/conf

install	: rebuild
	cp *.h $(SELIBPATH)/include
	cp $(LIBS) $(SELIBPATH)/lib

clean   :
	rm -rf $(INSTALL_PATH)
	/bin/rm -f *.o
	/bin/rm -f $(EXECUTABLE) $(TEST_EXEC) ${LIBS}
 
rebuild : clean all

cleanall : clean 
	rm -f *~

$(LIBS) : $(LIB_OBJS)
	$(AR) rcv $@ $^


predator: predator_main.o predator.o urlpool.o
	$(CPP) $(CPPFLAGS) -o $@ $^ $(LIB)

predator_test: predator_test.o urlpool.o predator.o
	$(CPP) $(CPPFLAGS) -o $@ $^ $(LIB)

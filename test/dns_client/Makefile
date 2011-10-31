WORKROOT=../../../public/
SELIBPATH=$(WORKROOT)/selib
LIBPATH=$(SELIBPATH)/lib
SPIDERLIBPATH=../../../spider/spider_lib
NLPLIBPATH=$(WORKROOT)/../nlp

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


LIB= -lpthread \
	-L$(SELIBPATH)/lib \
	-lnet \
	-lutil \

all : test_dns_client

OBJS= udpdns.o \
	  dns_client.o \
      test_dns_client.o \
      

clean :
	rm -rf *.o *~ test_dns_client
test_dns_client : $(OBJS) 
	$(CPP) $(CPPFLAGS) -o $@ $^ $(LIB)

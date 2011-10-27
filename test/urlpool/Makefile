WORKROOT=../../../public/
SELIBPATH=$(WORKROOT)/selib
LIBPATH=$(SELIBPATH)/lib
SPIDERLIBPATH=../../../spider/spider_lib
NLPLIBPATH=$(WORKROOT)/../nlp

CPP=g++
AR=ar

vpath %.h ../..
vpath %.cpp ../..

INCLUDE = -I./	\
	      -I$(SELIBPATH)/include \
		  -I$(SPIDERLIBPATH)/include/htmlcxx \
			-I$(SPIDERLIBPATH)/include \
		  -I$(NLPLIBPATH)/include

FLAGS=-g $(INCLUDES) -O2

LIB= 	-lpthread \
		  -L$(SELIBPATH)/lib \
		  -L$(SPIDERLIBPATH)/lib \
		  -L$(NLPLIBPATH)/lib \
		  -lurl \
		  -lextractor \
	      -lnet \
	      -lutil \
		  -ltse \
		  -L/usr/local/lib -llog4cxx \
	      -L/usr/local/lib -lapr-1\
	      -L/usr/local/lib -laprutil-1\
		-lurlpattern \
		-lcrypto	\
		-lcode	\

all : test_urlpool

OBJS= urlpool.o \
      test_urlpool.o

clean :
	rm -rf $(OBJS) test_urlpool
test_urlpool : $(OBJS) 
	$(CPP) $(FLAGS) -o $@ $^ $(LIB)

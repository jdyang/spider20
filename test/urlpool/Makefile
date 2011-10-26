CPP=g++
AR=ar

vpath %.h ../..
vpath %.cpp ../..

INCLUDES= -I./ 

FLAGS=-g $(INCLUDES) -O2

LIB= -lpthread

all : test_urlpool

OBJS= urlpool.o \
      test_urlpool.o

clean :
	rm -rf $(OBJS) test_urlpool
test_urlpool : $(OBJS) 
	$(CPP) $(FLAGS) -o $@ $^ $(LIB)

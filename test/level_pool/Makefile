CPP=g++
AR=ar

vpath %.h ../..
vpath %.cpp ../..

INCLUDES= -I./ \

FLAGS=-g $(INCLUDES)

LIB= -lpthread

all : test_level_pool

OBJS= ../../level_pool.o \
      test_level_pool.o

clean :
	rm -rf *.o *~ test_level_pool
test_level_pool : $(OBJS) 
	$(CPP) $(FLAGS) -o $@ $^ $(LIB)

CPP=g++
AR=ar

vpath %.h ../..
vpath %.cpp ../..

INCLUDES= -I./ \

FLAGS=-g $(INCLUDES)

LIB= -lpthread

all : test_spider_statis

OBJS= spider_statis.o \
      test_spider_statis.o

clean :
	rm -rf *.o *~ test_page_output
test_spider_statis : $(OBJS) 
	$(CPP) $(FLAGS) -o $@ $^ $(LIB)

CPP=g++
AR=ar

vpath %.h ../..
vpath %.cpp ../..

INCLUDES= -I./ \

FLAGS=-g $(INCLUDES)

LIB= -lpthread

all : test_selected_queue

OBJS= ../../selected_queue.o \
      test_selected_queue.o

clean :
	rm -rf *.o *~ test_selected_queue
test_selected_queue : $(OBJS) 
	$(CPP) $(FLAGS) -o $@ $^ $(LIB)

CPP=g++
AR=ar

vpath %.h ../..
vpath %.cpp ../..

INCLUDES= -I./ \

FLAGS=-g $(INCLUDES)

LIB= -lpthread

all : test_page_output

OBJS= page_output.o \
      test_page_output.o

clean :
	rm -rf *.o *~ test_page_output
test_page_output : $(OBJS) 
	$(CPP) $(FLAGS) -o $@ $^ $(LIB)

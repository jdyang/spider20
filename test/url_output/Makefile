CPP=g++
AR=ar


INCLUDES= -I./ \
          -I../../ \

FLAGS=-g $(INCLUDES)

LIB= -lpthread

all : test_url_output

OBJS= ../../url_output.o \
      test_url_output.o

clean :
	rm -rf *.o *~ test_url_output
test_url_output : $(OBJS) 
	$(CPP) $(FLAGS) -o $@ $^ $(LIB)

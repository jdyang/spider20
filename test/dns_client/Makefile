CPP=g++
AR=ar

vpath %.h ../..
vpath %.cpp ../..

INCLUDES= -I./ \

FLAGS=-g $(INCLUDES)

LIB= -lpthread

all : test_dns_client

OBJS= udpdns.o \
	  dns_client.o \
      test_dns_client.o \
      

clean :
	rm -rf *.o *~ test_page_output
test_dns_client : $(OBJS) 
	$(CPP) $(FLAGS) -o $@ $^ $(LIB)

PRGM=simulator
CC=g++
LDFLAGS=-lrt -lz
CFLAGS =-Wall 
COMPILE =$(CC) $(CPPFLAGS) $(CFLAGS) -c
LINKCC  =$(CC) $(CPPFLAGS) $(CFLAGS)
LIBS = -lpthread -lrt
SRCS := $(wildcard *.cpp)  
OBJS := $(patsubst %.cpp,%.o,$(SRCS))  
all:$(PRGM)  
$(PRGM):$(OBJS)
	$(LINKCC) -o ../bin/$(PRGM) $(LDFLAGS) $(LIBS) $(OBJS)
%.o:%.cpp
	$(COMPILE) $< -o $@ 
.PHONY:clean  
clean: 
	@-rm -f $(OBJS)  $(PRGM)
	@-rm -f *~
	@-rm -f core-*


COMPILER=g++

LIBS=
CFLAGS=
DEBUG=-g
TEST=-DTEST

#################################################################################

TARGET = picamserver
OBJS = picamserver.o ImageMgr.o producer_consumer.o util.o

$(TARGET): $(OBJS)
	$(COMPILER) $(DEBUG) -lpthread -o $@ $^

ImageMgr.o: ImageMgr.h util.h

%.o: %.cpp
	$(COMPILER) $(DEBUG) $(TEST) -std=c++0x -Wall -Wextra $(CFLAGS) -c -o $@ $<

dist:
	cp picamserver picamserver_SAVE
	strip picamserver

clean:
	rm -f $(OBJS) $(TARGET) core*
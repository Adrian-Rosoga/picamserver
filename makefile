COMPILER=g++

LIBS=-lpthread
CFLAGS=-Ofast
#DEBUG=-g
#TEST=-DTEST

#################################################################################

TARGET = picamserver
OBJS = picamserver.o ImageMgr.o producer_consumer.o util.o

$(TARGET): $(OBJS)
	$(COMPILER) $(DEBUG) $(LIBS) $(CFLAGS) -o $@ $^

ImageMgr.o: ImageMgr.h util.h producer_consumer.h
picamserver.o: ImageMgr.h util.h ThreadPool.h producer_consumer.h

%.o: %.cpp
	$(COMPILER) $(DEBUG) $(TEST) $(CFLAGS) -std=c++0x -Wall -Wextra -c -o $@ $<

dist:
	cp picamserver picamserver_SAVE
	strip picamserver

clean:
	rm -f $(OBJS) $(TARGET) core*

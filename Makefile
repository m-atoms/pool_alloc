APPNAME = pool_test

CC = gcc
CFLAGS = -I. -Wall
CFLAG_ADDS = -DVERBOSE -DFUNCTIONAL_TEST

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(CFLAG_ADDS)

$(APPNAME): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o
	rm -f $(APPNAME)

all: clean $(APPNAME)

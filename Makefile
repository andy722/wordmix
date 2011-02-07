CC=gcc
LD=gcc

TARGET=remix

CFLAGS=-O2 -DNDEBUG -DREENTRANT -D_REENTRANT
LDFLAGS=-s

#CFLAGS=-g -DDEBUG -DREENTRANT -D_REENTRANT
#LDFLAGS=-g

#CFLAGS=-pg -DNDEBUG -DREENTRANT -D_REENTRANT
#LDFLAGS=-pg
#CFLAGS=-O2 -DNDEBUG

LDFLAGS=-lpthread

OBJFILES=remix.o main.o wordlist.o

all:	remix

$(TARGET):	$(OBJFILES)
	$(LD) $(LDFLAGS) -o remix $(OBJFILES)

%.o:	%.c
	$(CC) $(CFLAGS) -c -o $@ $<

wordlist.c:	wordlist.txt
	./text2c wordlist.txt > wordlist.c

clean:
	rm -f *.o $(TARGET)

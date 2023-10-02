# Makefile for zmac

# Edit any defs which are wrong - these should be OK for Linux though.

CC = gcc

YACC = bison -y

MV = mv

ZIP = zip

CFLAGS = -O -Wall



all:	zmac

zmac:	zmac.c mio.c getoptn.c
	$(CC) $(CFLAGS) -o zmac zmac.c mio.c getoptn.c

zmac.c: zmac.y
	$(YACC) zmac.y
	$(MV) y.tab.c zmac.c



clean:
	$(RM) *.o y.tab.c zmac.c *~ zmac

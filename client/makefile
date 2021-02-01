macrosdir = ../
CC = gcc
CFLAGS = -std=c11 -Wall -Wconversion -Werror -Wextra -Wpedantic -fstack-protector-all -fpie -O -g -pthread -I$(macrosdir) -D_XOPEN_SOURCE=500
LDFLAGS = -lrt
VPATH = $(macrosdir)
objects = client_exe.o client.o
executable = client_exe

all: $(executable)

clean:
	$(RM) $(objects) $(executable)

$(executable): $(objects)
	$(CC) $(objects) $(LDFLAGS) -o $(executable)

client.o: client.c client.h MACROS.h
client_exe.o: client_exe.c client.h MACROS.h


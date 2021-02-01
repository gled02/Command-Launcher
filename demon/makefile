thread_pooldir = ../thread_pool/
macrosdir = ../
CC = gcc
CFLAGS = -std=c11 -Wall -Wconversion -Werror -Wextra -Wpedantic -fstack-protector-all -fpie -O -g -I$(macrosdir) -I$(thread_pooldir) -D_XOPEN_SOURCE=500
LDFLAGS1 = -pthread -lrt
LDFLAGS2 = 
VPATH = $(thread_pooldir):$(macrosdir)
objects1 = $(thread_pooldir)thread_pool.o  demon.o
objects2 = cpp.o
objects3 = lsl.o
objects4 = mypwd.o
executable1 = demon
executable2 = cpp
executable3 = lsl
executable4 = mypwd

all: $(executable1) $(executable2) $(executable3) $(executable4)


clean:
	$(RM) $(objects1) $(objects2) $(objects3) $(objects4) $(executable1) $(executable2) $(executable3) $(executable4)

$(executable1): $(objects1)
	$(CC) $(objects1) $(LDFLAGS1) -o $(executable1)
$(executable2): $(objects2)
	$(CC) $(objects2) $(LDFLAGS2) -o $(executable2)
$(executable3): $(objects3)
	$(CC) $(objects3) $(LDFLAGS1) -o $(executable3)
$(executable4): $(objects4)
	$(CC) $(objects4) $(LDFLAGS2) -o $(executable4) 

$(thread_managerdir)thread_manager.o: thread_pool.c thread_pool.h MACROS.h
demon_exe.o: demon.c thread_pool.h MACROS.h
cpp.o: cpp.c
lsl.o: lsl.c
mypwd.o: mypwd.c

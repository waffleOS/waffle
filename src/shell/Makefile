CFLAGS = -Wall -g

OBJS = mysh.o execute_shell.o

all: mysh test_execute

mysh: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test_execute: test_execute.o execute_shell.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean: 
	rm -f *.o *~ mysh

.PHONY: all clean


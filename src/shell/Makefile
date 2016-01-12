CFLAGS = -Wall -g

OBJS = mysh.o execute_shell.o

all: mysh test_execute parser_tests

mysh: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test_execute: test_execute.o execute_shell.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

parser_tests: parser_tests.o parse.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean: 
	rm -f *.o *~ mysh

.PHONY: all clean


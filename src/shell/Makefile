CFLAGS = -Wall -g

OBJS = mysh.o execute_shell.o parse.o parser_tests.o

all: mysh test_execute parser_tests

mysh: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test_execute: test_execute.o execute_shell.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: parser_tests.o parse.o test.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o *~ mysh test_execute test

.PHONY: all clean

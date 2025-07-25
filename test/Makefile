CC=gcc
RM=rm

all: gemini-test grok-example test

gemini-test: gemini-test.c ../src/hashmap.c
	$(CC) $^ -o gemini-test 

grok-example: grok-example.c ../src/hashmap.c
	$(CC) $^ -o grok-example `pkg-config --libs --cflags libxxhash`

test: test.c ../src/hashmap.c
	$(CC) $^ -o test `pkg-config --libs --cflags libxxhash`


clean:
	$(RM) gemini-test
	$(RM) grok-example
	$(RM) test

all:
	cc -fsanitize=address -Wall -O0 -g -o main main.c

test: all
	./main test.json

clean:
	rm main

.PHONY: all test clean

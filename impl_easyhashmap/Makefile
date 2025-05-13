FILE_NAME=main

CFLAGS=-Wall -Wextra -std=c11 -pedantic -g

all: $(FILE_NAME).c
	$(CC) $(CFLAGS) -o $(FILE_NAME) $(FILE_NAME).c
	./$(FILE_NAME)

clean:
	rm $(FILE_NAME)

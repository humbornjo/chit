CFLAGS=-Wall -Wextra -std=c11 -pedantic -g
FILE_NAME=himap

hash_teable: $(FILE_NAME).c
	$(CC) $(CFLAGS) -o $(FILE_NAME) $(FILE_NAME).c
	./$(FILE_NAME)

clean:
	rm $(FILE_NAME)

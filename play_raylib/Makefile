CC     = clang

CFLAGS = -Wall -Werror

OBJS = libraylib.a

LDLIBS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo

all: snake

snake: $(OBJS)
	$(CC) $(CFLAGS) -o Snake main.c $(OBJS) $(LDLIBS)

clean:
	make -C lib/external/raylib/src clean
	rm -f ./Snake ./*.o ./*.a

libraylib.a:
	make -C lib/external/raylib/src
	cp ./lib/external/raylib/src/libraylib.a ./

submodules:
	git submodule init
	git submodule update

.PHONY: all snake

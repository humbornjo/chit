CC         = clang

CFLAGS = -Wall -Werror

TARGET     := ray

# Build Target
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -I/opt/homebrew/include -L/opt/homebrew/lib -lraylib -o $(TARGET) main.c 

# Clean Target
.PHONY: clean
clean:
	rm -f $(TARGET)

# Phony target to prevent conflicts with files named "all" or "clean"
.PHONY: all
all: $(TARGET)

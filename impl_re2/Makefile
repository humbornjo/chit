# Makefile for RE2 regex implementation

CC = gcc
CFLAGS = -Wall -Wextra -g -O0
LDFLAGS =

# Target executable
TARGET = re2

# Default target (compiles and runs)
default: $(TARGET)
	./$(TARGET)

# Build re2 executable from re2.c and rune.h
$(TARGET): re2.c rune.h
	$(CC) $(CFLAGS) -o $(TARGET) re2.c $(LDFLAGS)

# Clean build artifacts
clean:
	rm -f $(TARGET) *.o

# Run test
test: $(TARGET)
	./$(TARGET)

# Phony targets
.PHONY: all clean test

# Help target
help:
	@echo "Available targets:"
	@echo "  all    - Build re2 executable (default)"
	@echo "  clean  - Remove build artifacts"
	@echo "  test   - Run re2 test"
	@echo "  help   - Show this help message"

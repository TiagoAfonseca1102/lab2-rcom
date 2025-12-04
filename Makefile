# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -O2

# Executable name
TARGET = download

# Source files
SRCS = download.c
OBJS = $(SRCS:.c=.o)

# Default rule
all: $(TARGET)

# Link executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile .c to .o
%.o: %.c download.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean object files and executable
clean:
	rm -f $(OBJS) $(TARGET)

# Phony tags
.PHONY: all clean


# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I.

# Linker flags
LDFLAGS = -lwayland-client -lm -lpthread

# Source files
SRCS = main.c async.c xdg-shell-protocol.c stb_image_impl.c

# Object files are placed in build directory
OBJDIR = build
OBJS = $(SRCS:%.c=$(OBJDIR)/%.o)

# Target executable name
TARGET = execthis

# Default target: build executable
all: $(TARGET)

# Link executable from object files
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile .c files to .o files in build directory
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create build directory if it does not exist
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Clean build artifacts
clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic
SECURITY_FLAGS = -fstack-protector-strong -D_FORTIFY_SOURCE=2 -O2 -pie -fPIE
LDFLAGS = -Wl,-z,relro,-z,now

# Program name
PROG = setperfbias

# Source files
SRC = setperfbias.c
OBJ = $(SRC:.c=.o)

# Installation directories
PREFIX = /usr
BINDIR = $(PREFIX)/bin

# Default target
all: $(PROG)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) $(SECURITY_FLAGS) -c $< -o $@

# Link object files into executable
$(PROG): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

# Install the program
install: $(PROG)
	install -d $(DESTDIR)$(BINDIR)
	install -o root -g root -m 4755 $(PROG) $(DESTDIR)$(BINDIR)/$(PROG)

# Clean build files
clean:
	rm -f $(PROG) $(OBJ)

# Check if source follows some basic security practices
security-check:
	@echo "Checking for common security issues..."
	@grep -n "[^_]printf" $(SRC) || echo "No unsafe printf found"
	@grep -n "strcpy" $(SRC) || echo "No strcpy found"
	@grep -n "strcat" $(SRC) || echo "No strcat found"
	@grep -n "gets" $(SRC) || echo "No gets found"

.PHONY: all clean install security-check
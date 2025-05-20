# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

#=======================================================================================================================
# Toolchain Configuration
#=======================================================================================================================

# C
export CFLAGS += -Werror -Wall -Wextra -std=c99
export CFLAGS += -D_POSIX_C_SOURCE=199309L

#=======================================================================================================================
# Build Artifacts
#=======================================================================================================================

# C source files.
export SRC_C := $(wildcard *.c)

# Object files.
export OBJ := $(SRC_C:.c=.o)

# Suffix for executable files.
export EXEC_SUFFIX := elf

# Compiles several object files into a binary.
export COMPILE_CMD = $(CC) $(CFLAGS) $@.o -o $(BINDIR)/$@.$(EXEC_SUFFIX) $(LIBS)

#=======================================================================================================================

# Builds everything.
all: sizes syscalls

make-dirs:
	mkdir -p $(BINDIR)

# Builds 'sizes' test.
sizes: make-dirs sizes.o
	$(COMPILE_CMD)

# Builds system call test.
syscalls: make-dirs syscalls.o
	$(COMPILE_CMD)

# Cleans up all build artifacts.
clean:
	@rm -rf $(OBJ)
	@rm -rf $(BINDIR)/sizes.$(EXEC_SUFFIX)
	@rm -rf $(BINDIR)/syscalls.$(EXEC_SUFFIX)

# Builds a C source file.
%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

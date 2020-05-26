#
# Compile python wrapper to call demo
#

# Declaration of variables
CC = gcc
CC_FLAGS = -shared -fPIC -I/usr/include/python2.7 \
	-Wall -Wextra -Wno-unused-parameter -O2 -g

# File names

# Main target
all: demopy.so

# Object files
demopy.so: pynif.o
	$(CC) $(CC_FLAGS) pynif.o demo_nif.o -o $@

pynif.o:	pynif.c
	$(CC) $(CC_FLAGS) -DPYNIFNAME=demopy -c $<

run: demopy.so
	python -c "import demo; print demo.hello(5)"

# To remove generated files
clean:
	rm -f *.so

.PHONY: all run clean

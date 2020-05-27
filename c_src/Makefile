#
# Compile python wrapper to call demo
#

# Declaration of variables
CC = gcc
CC_FLAGS = -shared -fPIC -I/usr/include/python2.7 \
	-Wall -Wextra -Wno-unused-parameter -O2 -g

# File names

# Main target
all: demopy.so demo2py.so demo3py.so

# demopy.so
demopy.so: demo_pynif.o
	$(CC) $(CC_FLAGS) -o $@ demo_pynif.o demo_nif.o 

demo_pynif.o:	pynif.c
	$(CC) $(CC_FLAGS) -o$@ -DPYNIFNAME=demopy -c $<

# demo2py.so
demo2py.so: demo2_pynif.o
	$(CC) $(CC_FLAGS) -o $@ demo2_pynif.o demo2_nif.o 

demo2_pynif.o:	pynif.c
	$(CC) $(CC_FLAGS) -o$@ -DPYNIFNAME=demo2py -c $<

# demo3py.so
demo3py.so: demo3_pynif.o
	$(CC) $(CC_FLAGS) -o $@ demo3_pynif.o demo3_nif.o 

demo3_pynif.o:	pynif.c
	$(CC) $(CC_FLAGS) -o$@ -DPYNIFNAME=demo3py -c $<

run: demopy.so
	python -c "import demopy; print demopy.hello(5)"

run: demo2py.so
	python -c "import demo2py; print demo2py.next(12)"

run: demo3py.so
	python -c "import demo3py; print demo3py.new_point(1.0, 2.2)"

# To remove generated files
clean:
	rm -f *.so

.PHONY: all run clean

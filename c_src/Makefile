#
# Compile python wrapper to call demo
#

# Declaration of variables

# PYTHON = python2

PYTHON = python3

PYTHON_INCLUDE = $(shell python3-config --includes)
PYTHON_LDFLAGS = $(shell python3-config --ldflags)
ERLDIR	:= $(shell erl -noshell -eval "io:format([126,115,126,110],[code:root_dir()])" -s erlang halt)
ERL_C_INCLUDE_DIR := "$(ERLDIR)/usr/include"

CC = gcc
CFLAGS = -fPIC -fno-common $(PYTHON_INCLUDE) -Wall -Wextra -Wno-unused-parameter -O2
LD_SHARED = gcc -shared

# File names

# Main target
all: nif.so demopy.so demo2py.so demo3py.so demo3dy.so

# nif.so
nif.so: nif.o nif_nif.o
	$(LD_SHARED) $(PYTHON_LDFLAGS) -o $@ nif.o nif_nif.o 

nif.o:	pynif.c
	$(CC) $(CFLAGS) -o$@ -DPYNIFNAME=nif -c $<

# demopy.so
demopy.so: demo_pynif.o demo_nif.o 
	$(LD_SHARED) $(PYTHON_LDFLAGS) -o $@ demo_pynif.o demo_nif.o 

demo_pynif.o:	pynif.c
	$(CC) $(CFLAGS) -o$@ -DPYNIFNAME=demopy -c $<

# demo2py.so
demo2py.so: demo2_pynif.o demo2_nif.o 
	$(LD_SHARED) $(PYTHON_LDFLAGS) -o $@ demo2_pynif.o demo2_nif.o 

demo2_pynif.o:	pynif.c
	$(CC) $(CFLAGS) -o$@ -DPYNIFNAME=demo2py -c $<

# demo3py.so
demo3py.so: demo3_pynif.o demo3_nif.o 
	$(LD_SHARED) $(PYTHON_LDFLAGS) -o $@ demo3_pynif.o demo3_nif.o 

demo3_pynif.o:	pynif.c
	$(CC) $(CFLAGS) -o$@ -DPYNIFNAME=demo3py -c $<

# NIFs
nif_nif.o:	nif_nif.c
	$(CC) -c -o $@ $(CFLAGS) -I$(ERL_C_INCLUDE_DIR) $<

demo_nif.o:	demo_nif.c
	$(CC) -c -o $@ $(CFLAGS) -I$(ERL_C_INCLUDE_DIR) $<

demo2_nif.o:	demo2_nif.c
	$(CC) -c -o $@ $(CFLAGS) -I$(ERL_C_INCLUDE_DIR) $<

demo3_nif.o:	demo3_nif.c
	$(CC) -c -o $@ $(CFLAGS) -I$(ERL_C_INCLUDE_DIR) $<

# demo3dy.so
# -Wl,-soname,demo3dy.so 
demo3dy.so: demo3dy_pynif.o
	$(LD_SHARED) $(PYTHON_LDFLAGS) -o $@ demo3dy_pynif.o

demo3dy_pynif.o:	pynif.c
	$(CC) $(CFLAGS) -o$@ -DPYNIFNAME=demo3dy -DPYNIFFILE=../priv/demo3.so -c $<

run1: demopy.so
	$(PYTHON) -c "import demopy; print(demopy.hello(5))"

run2: demo2py.so
	$(PYTHON) -c "import demo2py; print(demo2py.next(12))"

run3: demo3py.so
	$(PYTHON) -c "import demo3py; P = demo3py.new_point(1, 2); print(demo3py.get(P))"

run3dy: demo3dy.so
	$(PYTHON) -c "import sys; import ctypes; sys.setdlopenflags(sys.getdlopenflags()|ctypes.RTLD_GLOBAL); import demo3dy; P = demo3dy.new_point(1, 2); Q = demo3dy.new_point(3, 4); demo3dy.add(P, Q); print(demo3dy.get(Q))"

# To remove generated files
clean:
	rm -f *.so

.PHONY: all run clean

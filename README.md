# PyNIF - Call Erlang NIFs from Python

Work in progress.
A wrapper to be able to natively call erlang NIFS directly from
Python using a library that implement Erlangs nif_api layer.

Reuse your code!

# How to make examples

First compile the example nif modules from pynif directory

    rebar compile

Then compile examples

     cd c_src
     make

Run some examples

    make run1
    
    make run2
    
    make run3
    
    make run3dy
    
# How to use

# You have a nif object file?

If you can compile NIF source file to an object file you have the
option to link you nif object code the the pynif.o and get a
python library you can use as any extension to Python. You set
the variable PYNIFNAME to the python name you want to use.

    gcc -c -fPIC -DPYNIFNAME=demo pynif.c
    gcc -shared -odemo.so pynif.o demo.o

# You have a nif shared object .so file?

Since pynif have the option to load shared objects (from the shared object)
we can compile pynif to use a shared object. Use the PYNIFFILE to set
the name of the NIF file name to use.

    gcc -c -fPIC -DPYNIFNAME=demo PYNIFFILE=../priv/nifdemo.so pynif.c
    gcc -shared -opydemo.so pynif.o

You can try various ways with library paths and names to find a fit
that works with your project.

Have a look in c_src/Makefile for examples.



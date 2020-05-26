//
//  pynif: enif wrapper to allow python to call erlang nifs
//
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdarg.h>

#include "pynif.h"

#ifndef PYNIFNAME
#error "must define PYNIFNAME to module name"
#endif

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#define CAT_HELPER2(x,y) x ## y
#define CAT2(x,y) CAT_HELPER2(x,y)

#define MAX_PYNIF_FUNCS 16

#define UNUSED(var) (void)var
//
// INTEGER
//

int enif_get_int(ErlNifEnv* env, ERL_NIF_TERM term, int* ip)
{
    UNUSED(env);
    if (PyInt_Check(term)) {
	*ip = PyInt_AsLong(term);
	return 1;
    }
    return 0;
}

ERL_NIF_TERM enif_make_int(ErlNifEnv* env, int i)
{
    UNUSED(env);
    return PyInt_FromLong(i);
}

int enif_get_long(ErlNifEnv* env, ERL_NIF_TERM term, long* ip)
{
    UNUSED(env);
    if (PyInt_Check(term)) {
	*ip = PyInt_AsLong(term);
	return 1;
    }
    return 0;
}

ERL_NIF_TERM enif_make_long(ErlNifEnv* env, long i)
{
    UNUSED(env);
    return PyInt_FromLong(i);
}

int enif_get_uint(ErlNifEnv* env, ERL_NIF_TERM term, uint* ip)
{
    UNUSED(env);
    if (PyInt_Check(term)) {
	long val = PyInt_AsLong(term);
	if (val < 0) return 0;
	*ip = val;
	return 1;
    }
    return 0;
}

ERL_NIF_TERM enif_make_uint(ErlNifEnv* env, unsigned i)
{
    UNUSED(env);
    return PyInt_FromLong((long)i);
}

//
// FLOAT
//

int enif_get_double(ErlNifEnv* env, ERL_NIF_TERM term, double* dp)
{
    UNUSED(env);
    if (PyFloat_Check(term)) {
	*dp = PyFloat_AsDouble(term);
	return 1;
    }
    return 0;
}

ERL_NIF_TERM enif_make_double(ErlNifEnv* env, double d)
{
    UNUSED(env);    
    return PyFloat_FromDouble(d);
}


int enif_is_number(ErlNifEnv* env, ERL_NIF_TERM term)
{
    UNUSED(env);
    return PyInt_Check(term) || PyFloat_Check(term);
}

//
// ATOM
//

int enif_is_atom(ErlNifEnv* env, ERL_NIF_TERM term)
{
    UNUSED(env);
    return PyString_Check(term);
}

ERL_NIF_TERM enif_make_atom(ErlNifEnv* env, const char* name)
{
    UNUSED(env);
    return PyString_FromString(name);
}

ERL_NIF_TERM enif_make_atom_len(ErlNifEnv* env, const char* name, size_t len)
{
    UNUSED(env);
    return PyString_FromStringAndSize(name, len);
}

int enif_make_existing_atom(ErlNifEnv* env, const char* name, ERL_NIF_TERM* atom, ErlNifCharEncoding coding)
{
    UNUSED(env);
    UNUSED(coding);
    *atom = PyString_FromString(name);
    return 1;
}

int enif_get_atom(ErlNifEnv* env, ERL_NIF_TERM atom, char* buf, unsigned len,
		  ErlNifCharEncoding coding)
{
    UNUSED(env);
    UNUSED(coding);
    if (PyString_Check(atom)) {
	size_t str_len = PyString_Size(atom);
	char* str = PyString_AsString(atom);
	if (str_len < len) {
	    memcpy(buf, str, str_len);
	    buf[str_len] = '\0';
	    return str_len+1;
	}
    }
    return 0;
}

int enif_get_atom_length(ErlNifEnv* env, ERL_NIF_TERM atom, unsigned* len,
			 ErlNifCharEncoding coding)
{
    UNUSED(env);
    UNUSED(coding);
    if (PyString_Check(atom)) {
	size_t str_len = PyString_Size(atom);
	*len = str_len;
	return 1;
    }
    return 0;    
}

// 
// STRING
//

ERL_NIF_TERM enif_make_string_len(ErlNifEnv* env, const char* string, size_t len, ErlNifCharEncoding coding)
{
    UNUSED(env);
    UNUSED(coding);
    return PyString_FromStringAndSize(string, len);
}

// 
// ERROR
//
ERL_NIF_TERM enif_make_badarg(ErlNifEnv* env)
{
    return PyErr_Occurred();
}

//
// TUPLE 
//

int enif_is_tuple(ErlNifEnv* env, ERL_NIF_TERM term)
{
    UNUSED(env);
    return PyTuple_Check(term);
}

int enif_get_tuple(ErlNifEnv* env, ERL_NIF_TERM tpl, int* arity,
		   const ERL_NIF_TERM** array)
{
    if (PyTuple_Check(tpl)) {
	*arity = PyTuple_Size(tpl);
	// 3.8 does not expose the array!!! fixme!!!
	*array = ((PyTupleObject*)tpl)->ob_item;
	return 1;
    }
    return 0;
}

ERL_NIF_TERM enif_make_tuple_from_array(ErlNifEnv* env,
					const ERL_NIF_TERM arr[], unsigned cnt)
{
    UNUSED(env);    
    ERL_NIF_TERM tuple = PyTuple_New(cnt);
    unsigned i;

    for (i = 0; i < cnt; i++) 
	PyTuple_SET_ITEM(tuple, i, arr[i]);
    return tuple;
}
    
ERL_NIF_TERM enif_make_tuple(ErlNifEnv* env, unsigned cnt, ...)
{
    UNUSED(env);    
    va_list ap;
    ERL_NIF_TERM tuple = PyTuple_New(cnt);
    unsigned i = 0;

    va_start(ap, cnt);
    while(i < cnt) {
	PyTuple_SET_ITEM(tuple, i, va_arg(ap, ERL_NIF_TERM));
	i++;
    }
    va_end(ap);    
    return tuple;
}
    

ERL_NIF_TERM enif_make_tuple1(ErlNifEnv* env,
			      ERL_NIF_TERM e1)
{
    UNUSED(env);
    return enif_make_tuple(env, 1, e1);
}

ERL_NIF_TERM enif_make_tuple2(ErlNifEnv* env,
			      ERL_NIF_TERM e1,
			      ERL_NIF_TERM e2)
{
    UNUSED(env);
    return enif_make_tuple(env, 2, e1, e2);
}

ERL_NIF_TERM enif_make_tuple3(ErlNifEnv* env,
			      ERL_NIF_TERM e1,
			      ERL_NIF_TERM e2,
			      ERL_NIF_TERM e3)
{
    UNUSED(env);
    return enif_make_tuple(env, 3, e1, e2, e3);
}

ERL_NIF_TERM enif_make_tuple4(ErlNifEnv* env,
			      ERL_NIF_TERM e1,
			      ERL_NIF_TERM e2,
			      ERL_NIF_TERM e3,
			      ERL_NIF_TERM e4)
{
    UNUSED(env);
    return enif_make_tuple(env, 4, e1, e2, e3, e4);
}

ERL_NIF_TERM enif_make_tuple5(ErlNifEnv* env,
			      ERL_NIF_TERM e1,
			      ERL_NIF_TERM e2,
			      ERL_NIF_TERM e3,
			      ERL_NIF_TERM e4,
			      ERL_NIF_TERM e5)
{
    UNUSED(env);
    return enif_make_tuple(env, 5, e1, e2, e3, e4, e5);
}

ERL_NIF_TERM enif_make_tuple6(ErlNifEnv* env,
			      ERL_NIF_TERM e1,
			      ERL_NIF_TERM e2,
			      ERL_NIF_TERM e3,
			      ERL_NIF_TERM e4,
			      ERL_NIF_TERM e5,
			      ERL_NIF_TERM e6)
{
    UNUSED(env);
    return enif_make_tuple(env, 6, e1, e2, e3, e4, e5, e6);
}

ERL_NIF_TERM enif_make_tuple7(ErlNifEnv* env,
			      ERL_NIF_TERM e1,
			      ERL_NIF_TERM e2,
			      ERL_NIF_TERM e3,
			      ERL_NIF_TERM e4,
			      ERL_NIF_TERM e5,
			      ERL_NIF_TERM e6,
			      ERL_NIF_TERM e7)
{
    UNUSED(env);
    return enif_make_tuple(env, 7, e1, e2, e3, e4, e5, e6, e7);
}

ERL_NIF_TERM enif_make_tuple8(ErlNifEnv* env,
			      ERL_NIF_TERM e1,
			      ERL_NIF_TERM e2,
			      ERL_NIF_TERM e3,
			      ERL_NIF_TERM e4,
			      ERL_NIF_TERM e5,
			      ERL_NIF_TERM e6,
			      ERL_NIF_TERM e7,
			      ERL_NIF_TERM e8)
{
    UNUSED(env);
    return enif_make_tuple(env, 8, e1, e2, e3, e4, e5, e6, e7, e8);
}

ERL_NIF_TERM enif_make_tuple9(ErlNifEnv* env,
			      ERL_NIF_TERM e1,
			      ERL_NIF_TERM e2,
			      ERL_NIF_TERM e3,
			      ERL_NIF_TERM e4,
			      ERL_NIF_TERM e5,
			      ERL_NIF_TERM e6,
			      ERL_NIF_TERM e7,
			      ERL_NIF_TERM e8,
			      ERL_NIF_TERM e9)
{
    UNUSED(env);
    return enif_make_tuple(env, 9, e1, e2, e3, e4, e5, e6, e7, e8, e9);
}

//
// LIST
//

int enif_is_list(ErlNifEnv* env, ERL_NIF_TERM term)
{
    UNUSED(env);
    return PyList_Check(term);
}

int enif_get_list_length(ErlNifEnv* env, ERL_NIF_TERM term, unsigned* len)
{
    UNUSED(env);
    if (PyList_Check(term)) {
	*len = PyList_Size(term);
	return 1;
    }
    return 0;
}

int enif_is_empty_list(ErlNifEnv* env, ERL_NIF_TERM term)
{
    if (!PyList_Check(term))
	return 0;
    return PyList_Size(term) == 0;
}

ERL_NIF_TERM enif_make_list(ErlNifEnv* env, unsigned cnt, ...)
{
    UNUSED(env);
    va_list ap;
    ERL_NIF_TERM list = PyList_New(cnt);
    unsigned i = 0;

    va_start(ap, cnt);
    while(i < cnt) {
	PyList_SET_ITEM(list, i, va_arg(ap, ERL_NIF_TERM));
	i++;
    }
    va_end(ap);
    return list;
}

ERL_NIF_TERM enif_make_list_from_array(ErlNifEnv* env,
				       const ERL_NIF_TERM arr[], unsigned cnt)
{
    UNUSED(env);
    ERL_NIF_TERM list = PyList_New(cnt);
    unsigned i;
    
    for (i = 0; i < cnt; i++) 
	PyList_SET_ITEM(list, i, arr[i]);
    return list;    
}

// FIXME: Probably VERY slow!
int enif_get_list_cell(ErlNifEnv* env, ERL_NIF_TERM term,
		       ERL_NIF_TERM* head, ERL_NIF_TERM* tail)
{
    if (PyList_Check(term)) {
	Py_ssize_t length = PyList_Size(term);
	if (length > 0) {
	    *head = PyList_GetItem(term, 0);
	    if (length > 1)
		*tail = PyList_GetSlice(term, 1, length);
	    else
		*tail = PyList_New(0);
	    return 1;
	}
    }
    return 0;
}

// FIXME: since Python lists are really vectors we should try to make
// this a bit differently. Maybe just promote create lists with make_list ...
ERL_NIF_TERM enif_make_list_cell(ErlNifEnv* env, ERL_NIF_TERM hd, ERL_NIF_TERM tl)
{
    UNUSED(env);
    if (PyList_Check(tl)) {
	PyList_Insert(tl, 0, hd);
	return tl;
    }
    else {
	// dotted pair does not exist in Python
	return enif_make_tuple2(env, hd, tl);
    }
}

//
// MAP
//

int enif_is_map(ErlNifEnv* env, ERL_NIF_TERM term)
{
    UNUSED(env);
    return PyDict_Check(term);
}

//
// MEMORY
//
void* enif_alloc(size_t size)
{
    return PyMem_Malloc(size);
}

void enif_free(void* ptr)
{
    PyMem_Free(ptr);
}

void* enif_realloc(void* ptr, size_t size)
{
    return PyMem_Realloc(ptr, size);
}

//
// Modules
// if STATIC_ERLANG_NIF was set at build time then the init function
// is call <modname>_nif_init(ERL_NIF_INIT_ARGS)
//
static ErlNifEntry* nif_entry = NULL;
static ErlNifEnv nif_env;

#define PYNIF_BODY(fi, me, args)				\
    PyObject** argv = ((PyTupleObject *)(args))->ob_item;	\
    int argc = PyTuple_Size((args));				\
    nif_env.self = (me);					\
    return (*nif_entry->funcs[(fi)].fptr)(&nif_env, argc, argv)

#define PYNIF_FUNC(fi)				\
    static PyObject* pynif_##fi(PyObject* self, PyObject* args) { PYNIF_BODY((fi), self, args); }
    
PYNIF_FUNC(0)
PYNIF_FUNC(1)
PYNIF_FUNC(2)
PYNIF_FUNC(3)
PYNIF_FUNC(4)
PYNIF_FUNC(5)
PYNIF_FUNC(6)
PYNIF_FUNC(7)
PYNIF_FUNC(8)
PYNIF_FUNC(9)
PYNIF_FUNC(10)
PYNIF_FUNC(11)
PYNIF_FUNC(12)
PYNIF_FUNC(13)
PYNIF_FUNC(14)
PYNIF_FUNC(15)

static PyCFunction pynif_func[MAX_PYNIF_FUNCS] =
{
    pynif_0, pynif_1, pynif_2, pynif_3, 
    pynif_4, pynif_5, pynif_6, pynif_7,
    pynif_8, pynif_9, pynif_10, pynif_11, 
    pynif_12, pynif_13, pynif_14, pynif_15,
};

extern ErlNifEntry* nif_init(void);

PyMODINIT_FUNC
CAT2(init,PYNIFNAME)(void)
{
    // now convert all funcs into PyMethodDef array
    PyMethodDef* methods;
    int i;

    nif_entry = nif_init();
    // FIXME: call "load" function if defined!

    // fprintf(stderr, "nif_entry: name = %s\n", nif_entry->name);
    // fprintf(stderr, "nif_entry: num_of_funcs = %d\n", nif_entry->num_of_funcs);
    
    methods = malloc(sizeof(PyMethodDef)*(nif_entry->num_of_funcs+1));
    memset(methods, 0, sizeof(PyMethodDef)*(nif_entry->num_of_funcs+1));
    for (i = 0; i < nif_entry->num_of_funcs; i++) {
	methods[i].ml_name = nif_entry->funcs[i].name;
	methods[i].ml_meth = pynif_func[i];
	methods[i].ml_flags =  METH_VARARGS;
	methods[i].ml_doc = "PyNif function";
    }
    Py_InitModule(STRINGIFY(PYNIFNAME), methods);
}

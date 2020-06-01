//
//  pynif: enif wrapper to allow python to call erlang nifs
//

#include "pynif.h"

#ifndef WIN32
#include <stdarg.h>
#include <stdint.h>
#include <dlfcn.h>
#else
#include <windows.h>
#endif

#ifndef PYNIFNAME
#error "must define PYNIFNAME to module name"
#endif

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#define CAT_HELPER2(x,y) x ## y
#define CAT2(x,y) CAT_HELPER2(x,y)

// FIXME dynamic how?
#define MAX_PYNIF_FUNCS 256

#define UNUSED(var) (void)var
#define DBG(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
// #define DBG(fmt, ...)
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
    else if (PyLong_Check(term)) {
	*ip = PyLong_AsLong(term);
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
    else if (PyLong_Check(term)) {
	*ip = PyLong_AsLong(term);
	return 1;
    }    
    return 0;
}

ERL_NIF_TERM enif_make_long(ErlNifEnv* env, long i)
{
    UNUSED(env);
    return PyInt_FromLong(i);
}


int enif_get_ulong(ErlNifEnv* env, ERL_NIF_TERM term, unsigned long* up)
{
    UNUSED(env);
    if (PyInt_Check(term)) {
	*up = PyInt_AsUnsignedLongMask(term);
	return 1;
    }
    else if (PyLong_Check(term)) {
	*up = PyLong_AsUnsignedLong(term);
	return 1;
    }    
    return 0;
}

// FIXME: generate PyLong if needed?
ERL_NIF_TERM enif_make_ulong(ErlNifEnv* env, unsigned long i)
{
    UNUSED(env);
    return PyInt_FromLong((long)i);
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
    else if (PyLong_Check(term)) {
	long val = PyLong_AsLong(term);
	if (val < 0) return 0;
	*ip = val;
	return 1;
    }
    return 0;
}

// FIXME: generate PyLong if needed?
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
// ATOM  Fixme is it possible to subclass PyInt?
//

int enif_is_atom(ErlNifEnv* env, ERL_NIF_TERM term)
{
    UNUSED(env);
    return PyString_Check(term);
}

static int  make_atom(ErlNifEnv* env, const char* name, int existing,
		      ERL_NIF_TERM* atomp)
{
    PyObject* atm;

    if ((atm = PyDict_GetItemString(env->atoms, name)) == NULL) {
	PyObject* obj;
	if (existing) return 0;
	    
	if (env->atom_index >= env->atom_table_size) {
	    size_t new_size = env->atom_table_size+EXPAND_ATOM_TABLE_SIZE;
	    PyObject** tab = PyMem_Realloc(env->atom_table,
					   new_size*sizeof(PyObject*));
	    env->atom_table = tab;
	    env->atom_table_size = new_size;
	}
	if (PyModule_Check(env->self))
	    PyModule_AddIntConstant(env->self, name, env->atom_index);
	atm = PyInt_FromLong(env->atom_index);
	obj = PyString_FromString(name);
	env->atom_table[env->atom_index++] = obj;
	PyDict_SetItemString(env->atoms, name, atm);
	*atomp = atm;
	return 1;	
    }
    *atomp = atm;
    return 1;
}

ERL_NIF_TERM enif_make_atom(ErlNifEnv* env, const char* name)
{
    ERL_NIF_TERM atom;
    make_atom(env, name, 0, &atom);
    return atom;
}

ERL_NIF_TERM enif_make_atom_len(ErlNifEnv* env, const char* name, size_t len)
{
    ERL_NIF_TERM atom;
    char name2[len+1];
    memcpy(name2, name, len);
    name2[len] = '\0';
    make_atom(env, name2, 0, &atom);
    return atom;
}

int enif_make_existing_atom(ErlNifEnv* env, const char* name, ERL_NIF_TERM* atom, ErlNifCharEncoding coding)
{
    UNUSED(coding);
    return make_atom(env, name, 1, atom);
}

static int get_string(PyObject* string, char* buf, unsigned len,
		      ErlNifCharEncoding coding)
{
    UNUSED(coding);
    size_t str_len = PyString_Size(string);
    char* str = PyString_AsString(string);
    if (str_len < len) {
	memcpy(buf, str, str_len);
	buf[str_len] = '\0';
	return str_len+1;
    }
    return 0;
}

int enif_get_atom(ErlNifEnv* env, ERL_NIF_TERM atom, char* buf, unsigned len,
		  ErlNifCharEncoding coding)
{
    UNUSED(env);
    UNUSED(coding);
    if (PyBool_Check(atom)) {
	if (atom == Py_False) {
	    if (len >= 6) {
		strcpy(buf, "false");
		return 6;
	    }
	}
	else if (atom == Py_True) {
	    if (len >= 5) {
		strcpy(buf, "true");
		return 4;
	    }
	}
    }
    else if (PyInt_Check(atom)) {
	long i = PyInt_AsLong(atom);
	if ((i < 0) || (i >= (long)env->atom_index))
	    return 0;
	else if (i == 0) {
	    if (len >= 6) {
		strcpy(buf, "false");
		return 6;
	    }
	}
	else if (i == 1) {
	    if (len >= 5) {
		strcpy(buf, "true");
		return 4;
	    }
	}
	else {
	    atom = env->atom_table[i];
	    return get_string(atom, buf, len, coding);
	}
    }
    else if (PyString_Check(atom)) {
	return get_string(atom, buf, len, coding);
    }
    return 0;
}

int enif_get_atom_length(ErlNifEnv* env, ERL_NIF_TERM atom, unsigned* len,
			 ErlNifCharEncoding coding)
{
    UNUSED(env);
    UNUSED(coding);

    if (PyBool_Check(atom)) {
	if (atom == Py_False) {
	    *len = 5;
	    return 1;
	}
	else if (atom == Py_True) {
	    *len = 4;
	    return 1;
	}
    }
    else if (PyInt_Check(atom)) {
	long i = PyInt_AsLong(atom);
	if ((i < 0) || (i >= (long)env->atom_index))
	    return 0;
	if (i == 0) {
	    *len = 5;
	    return 1;
	}
	else if (i == 1) {
	    *len = 4;
	    return 1;
	}
	else {
	    *len = PyString_Size(env->atom_table[i]);
	    return 1;
	}
    }
    else if (PyString_Check(atom)) {
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
    PyErr_SetString(PyExc_TypeError, "badarg");
    return NULL; // fixme?
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

int enif_inspect_binary(ErlNifEnv* end, ERL_NIF_TERM bin_term,
			ErlNifBinary* bin)
{
    if (PyByteArray_Check(bin_term)) {
	bin->size = PyByteArray_Size(bin_term);
	bin->data = (unsigned char *) PyByteArray_AsString(bin_term);
	bin->allocated = 0;
	bin->ref_bin = bin_term;
	return 1;
    }
    return 0;
}

ErlNifEnv* enif_alloc_env(void)
{
    ErlNifEnv* env = (ErlNifEnv*) PyMem_Malloc(sizeof(ErlNifEnv));
    memset(env, 0, sizeof(ErlNifEnv));
    return env;
}

void enif_clear_env(ErlNifEnv* env)
{
    memset(env, 0, sizeof(ErlNifEnv));
}


unsigned char* enif_make_new_binary(ErlNifEnv* env,size_t size,
				    ERL_NIF_TERM* termp)
{
    PyObject* obj = PyByteArray_FromStringAndSize("\0", 1);
    if (PyByteArray_Resize(obj, size) < 0)
	return NULL;
    *termp = obj;
    return (unsigned char*) PyByteArray_AS_STRING(obj);
}

int enif_compare_monitors(const ErlNifMonitor* a, const ErlNifMonitor* b)
{
    return 0;
}

int enif_monitor_process(ErlNifEnv* env, void* obj, const ErlNifPid* pid,
			 ErlNifMonitor *monitor)
{
    return 0;
}


ERL_NIF_TERM enif_raise_exception(ErlNifEnv *env, ERL_NIF_TERM reason)
{
    UNUSED(env);
    UNUSED(reason);
    PyErr_SetString(PyExc_TypeError, "exception");
    return NULL;    // fixme?
}

// FIXME: put msg into a message box under the module object (key on thread?)
int enif_send(ErlNifEnv* env, const ErlNifPid* to_pid, ErlNifEnv* msg_env, ERL_NIF_TERM msg)
{
    return 0;
}

ErlNifPid* enif_self(ErlNifEnv* caller_env, ErlNifPid* pid)
{
    UNUSED(caller_env);
    pid->thread_ident = PyThread_get_thread_ident();
    return pid;
}


//
// MAP
//

int enif_is_map(ErlNifEnv* env, ERL_NIF_TERM term)
{
    UNUSED(env);
    return PyDict_Check(term);
}

int enif_make_map_from_arrays(ErlNifEnv *env, ERL_NIF_TERM keys[], ERL_NIF_TERM values[], size_t cnt, ERL_NIF_TERM *map_out)
{
    UNUSED(env);
    PyObject* dict;
    int i;

    if ((dict = PyDict_New()) == NULL)
	return 0;
    for (i = 0; i < (int)cnt; i++) {
	if (PyDict_SetItem(dict, keys[i], values[i]) < 0)
	    return 0;
    }
    *map_out = dict;
    return 1;
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

int enif_alloc_binary(size_t size, ErlNifBinary* bin)
{
    if ((bin->data = PyMem_Malloc(size)) != NULL) {
	bin->size = size;
	bin->allocated = 1;
	bin->ref_bin = NULL;
	return 1;
    }
    bin->allocated = 0;
    return 0;
}

void enif_release_binary(ErlNifBinary* bin)
{
    if (bin->allocated) {
	if (bin->data != NULL)
	    PyMem_Free(bin->data);
	bin->data = NULL;
	bin->size = 0;
	bin->allocated = 0;
	bin->ref_bin = NULL;	
    }
}

int enif_inspect_iolist_as_binary(ErlNifEnv* env, ERL_NIF_TERM term,
				  ErlNifBinary* bin)
{
    if (PyByteArray_Check(term)) {
	bin->size = PyByteArray_Size(term);
	bin->data = (unsigned char*) PyByteArray_AsString(term);
	bin->allocated = 0;
	bin->ref_bin = term;
	return 1;	
    }
    else if (PyString_Check(term)) {
	bin->size = PyString_Size(term);
	bin->data = (unsigned char*) PyString_AsString(term);
	bin->allocated = 0;
	bin->ref_bin = term;
	return 1;	
    }
    else if (PyList_Check(term)) {
	ssize_t length = PyList_Size(term); // Fixme: recursive!
	unsigned char bytes[length];
	ssize_t i;

	// check that all Items are in byte range
	
	// Fixme: recursive!
	for (i = 0; i < length; i++) {
	    PyObject* item = PyList_GetItem(term, i);
	    if (!PyInt_Check(item))
		return 0;
	    else {
		long v = PyInt_AsLong(item);
		if ((v >= 0) && (v < 256))
		    bytes[i] = v;
		else
		    return 0;
	    }
	}
	enif_alloc_binary(length, bin);
	memcpy(bin->data, bytes, length);
	return 1;
    }
    return 0;    
}

ERL_NIF_TERM enif_make_binary(ErlNifEnv* env, ErlNifBinary* bin)
{
    UNUSED(env);
    if (bin->allocated) {
	PyObject* obj = PyByteArray_FromStringAndSize((const char*)bin->data, bin->size);
	bin->allocated = 0;
	PyMem_Free(bin->data);
	return obj;
    }
    else if ((bin->ref_bin != NULL) && PyByteArray_Check(bin->ref_bin))
	return bin->ref_bin;  // RefCount?
    else {
	return PyByteArray_FromStringAndSize((const char*)bin->data, bin->size);
    }
}

//
// THREAD/LOCK
//
ErlNifMutex* enif_mutex_create(char *name)
{
    UNUSED(name);
    return (ErlNifMutex*) PyThread_allocate_lock();
}

void enif_mutex_destroy(ErlNifMutex *mtx)
{
    PyThread_free_lock((PyThread_type_lock)mtx);
}

int enif_mutex_trylock(ErlNifMutex *mtx)
{
    return PyThread_acquire_lock((PyThread_type_lock)mtx, NOWAIT_LOCK);
}

void enif_mutex_lock(ErlNifMutex *mtx)
{
    (void) PyThread_acquire_lock((PyThread_type_lock)mtx, WAIT_LOCK);
}

void enif_mutex_unlock(ErlNifMutex *mtx)
{
    PyThread_release_lock((PyThread_type_lock)mtx);
}


ErlNifRWLock* enif_rwlock_create(char *name)
{
    UNUSED(name);
    return (ErlNifRWLock*) PyThread_allocate_lock();
}

void enif_rwlock_destroy(ErlNifRWLock *rwlck)
{
    PyThread_free_lock((PyThread_type_lock)rwlck);    
}

int enif_rwlock_tryrlock(ErlNifRWLock *rwlck)
{
    return PyThread_acquire_lock((PyThread_type_lock)rwlck, NOWAIT_LOCK);
}

void enif_rwlock_rlock(ErlNifRWLock *rwlck)
{
    (void) PyThread_acquire_lock((PyThread_type_lock)rwlck, WAIT_LOCK);
}

void enif_rwlock_runlock(ErlNifRWLock *rwlck)
{
    PyThread_release_lock((PyThread_type_lock)rwlck);
}

int enif_rwlock_tryrwlock(ErlNifRWLock *rwlck)
{
    return PyThread_acquire_lock((PyThread_type_lock)rwlck, NOWAIT_LOCK);    
}

void enif_rwlock_rwlock(ErlNifRWLock *rwlck)
{
    (void) PyThread_acquire_lock((PyThread_type_lock)rwlck, WAIT_LOCK);    
}

void enif_rwlock_rwunlock(ErlNifRWLock *rwlck)
{
    PyThread_release_lock((PyThread_type_lock)rwlck);
}    

int enif_tsd_key_create(char *name, ErlNifTSDKey *key)
{
    UNUSED(name);
    *key = PyThread_create_key();
    return 0;
}

void enif_tsd_key_destroy(ErlNifTSDKey key)
{
    PyThread_delete_key(key);
}
    
void enif_tsd_set(ErlNifTSDKey key, void *data)
{
    (void) PyThread_set_key_value(key, data);
}
    
void* enif_tsd_get(ErlNifTSDKey key)
{
    return PyThread_get_key_value(key);
}

// I/O
int enif_fprintf(FILE* filep, const char *format, ...)
{
    va_list ap;
    int r;
    
    va_start(ap, format);
    r = vfprintf(filep, format, ap);
    va_end(ap);
    return r;
}

//
// ADMIN
//
void* enif_priv_data(ErlNifEnv* env)
{
    return env->priv_data;
}


typedef struct _resource_t
{
    PyObject_VAR_HEAD
    ErlNifEnv* env;
    uint8_t data[0];
} Resource;

// Erlang (void*) => PyVarObject (Resource)
#define OBJ_TO_RESOURCE(obj) ((Resource*)(((uint8_t*)obj) - sizeof(Resource)))
// PyVarObject (Resource*) to Erlang (void*)
#define RESOURCE_TO_OBJ(ptr) ((void*) &((Resource*)(ptr))->data[0])

// FIXME: not really working
typedef struct _resource_type_t {
    PyTypeObject tp;                  // : PyVarObject
    ErlNifResourceTypeInit ini;
} ResourceType;

static void resource_dealloc(PyObject* obj)
{
    Resource* ptr = (Resource*) obj;
    ResourceType* rtp = (ResourceType*) ptr->ob_type;
    (*rtp->ini.dtor)(ptr->env, RESOURCE_TO_OBJ(ptr));
}


ErlNifResourceType* enif_open_resource_type_x(
    ErlNifEnv* env,
    const char* name_str,
    const ErlNifResourceTypeInit* init,
    ErlNifResourceFlags flags,
    ErlNifResourceFlags* tried)
{
    ResourceType* rtp = enif_alloc(sizeof(ResourceType));

    memset(rtp, 0, sizeof(ResourceType));

    rtp->tp.ob_size = 1;  // for dtor
    rtp->tp.tp_name = name_str;
    rtp->tp.tp_basicsize = sizeof(Resource);
    rtp->tp.tp_itemsize  = 1;  // since it is a byte array (any struture)!
    rtp->tp.tp_flags = Py_TPFLAGS_DEFAULT;
    rtp->tp.tp_new = PyType_GenericNew;
    rtp->tp.tp_dealloc = resource_dealloc;
    // store callbacks
    rtp->ini = *init;

    if (PyType_Ready((PyTypeObject*) rtp) < 0) {
	enif_fprintf(stderr, "PyType_Ready failed\n");
    }
    // enif_fprintf(stderr, "ob_size = %ld\r\n", rtp->tp.ob_size);

    if (flags & ERL_NIF_RT_CREATE)
	*tried = ERL_NIF_RT_CREATE;
    return (ErlNifResourceType*)rtp;    
}


ErlNifResourceType* enif_open_resource_type(ErlNifEnv* env,
					    const char* module_str,
					    const char* name_str,
					    void (*dtor)(ErlNifEnv*,void *),
					    ErlNifResourceFlags flags,
					    ErlNifResourceFlags* tried)
{
    ResourceType* rtp = enif_alloc(sizeof(ResourceType));
    ErlNifResourceTypeInit init;
    memset(rtp, 0, sizeof(ResourceType));
    memset(&init, 0, sizeof(ErlNifResourceTypeInit));
    init.dtor = dtor;
    return enif_open_resource_type_x(env, name_str, &init,
				     flags, tried);
}

void* enif_alloc_resource(ErlNifResourceType* type, size_t size)
{
    Resource* ptr = PyObject_NewVar(Resource, type, size);
    Py_INCREF((PyObject*) ptr);    
    return RESOURCE_TO_OBJ(ptr);
}

void enif_release_resource(void* obj)
{
    Resource* ptr = OBJ_TO_RESOURCE(obj);
    Py_DECREF((PyObject*) ptr);
}

void enif_keep_resource(void* obj)
{
    Resource* ptr = OBJ_TO_RESOURCE(obj);
    Py_INCREF((PyObject*) ptr);
}

ERL_NIF_TERM enif_make_resource(ErlNifEnv* env, void* obj)
{
    return (PyObject*)OBJ_TO_RESOURCE(obj);
}
    
int enif_get_resource(ErlNifEnv* env, ERL_NIF_TERM term,
		      ErlNifResourceType* type, void** objp)
{
    if (term && (term->ob_type == type)) {
	*objp = RESOURCE_TO_OBJ(term);
	return 1;
    }
    return 0;
}

size_t enif_sizeof_resource(void* obj)
{
    Resource* ptr = OBJ_TO_RESOURCE(obj);
    return Py_SIZE(ptr);
}

static char* format_ext_tag(ErlExtTag tag)
{
    switch(tag) {
    case MAGIC: return "MAGIC";
    case SMALL_ATOM: return "SMALL_ATOM";
    case ATOM: return "ATOM";
    case BINARY: return "BINARY";
    case SMALL_INTEGER: return "SMALL_INTEGER";
    case INTEGER: return "INTEGER";
    case SMALL_BIG: return "SMALL_BIG";
    case LARGE_BIG: return "LARGE_BIG";
    case FLOAT: return "FLOAT";
    case NEW_FLOAT: return "NEW_FLOAT";
    case REFERENCE: return "REFERENCE";
    case NEW_REFERENCE: return "NEW_REFERENCE";
    case NEWER_REFERENCE: return "NEWER_REFERENCE";
    case STRING: return "STRING";
    case LIST: return "LIST";
    case SMALL_TUPLE: return "SMALL_TUPLE";
    case LARGE_TUPLE: return "LARGE_TUPLE";
    case NIL: return "NIL";
    case MAP: return "MAP";
    default: return "UNSUPPORTED";
    }
}

static inline uint8_t get_uint8(unsigned char* ptr)
{
    uint8_t v = (ptr[0]);
    return v;
}

static inline uint16_t get_uint16(unsigned char* ptr)
{
    uint16_t v = ((ptr[0]<<8)|ptr[1]);
    return v;
}

static inline uint32_t get_uint32(unsigned char* ptr)
{
    uint32_t v = ((ptr[0]<<24)|(ptr[1]<<16)|(ptr[2]<<8)|ptr[3]);
    return v;
}

static inline int32_t get_int32(unsigned char* ptr)
{
    uint32_t v = ((ptr[0]<<24)|(ptr[1]<<16)|(ptr[2]<<8)|ptr[3]);
    return (int32_t) v;
}

static inline void put_uint8(unsigned char* ptr, uint8_t v)
{
    ptr[0] = v;
}

static inline void put_uint16(unsigned char* ptr, uint16_t v)
{
    ptr[0] = (v >> 8);
    ptr[1] = v;
}

static inline void put_uint32(unsigned char* ptr, uint32_t v)
{
    ptr[0] = (v >> 24);
    ptr[1] = (v >> 16);
    ptr[2] = (v >> 8);
    ptr[3] = v;
}

static inline void put_int32(unsigned char* ptr, int32_t v)
{
    put_uint32(ptr, (uint32_t) v);
}

// at least one byte (that is >= 1)
static ssize_t bytesize_of_ulong(unsigned long v)
{
    ssize_t size = 1;
    v >>= 8;
    while(v) {
	size++;
	v >>= 8;
    }
    return size;
}

// put little endian ulong (at least one byte!)
static size_t encode_ulong(unsigned char* ptr, unsigned long v)
{
    size_t size = 1;
    *ptr++ = v;
    v >>= 1;
    while(v) {
	*ptr++ = v;
	v >>= 1;
	size++;
    }
    return size;
}

// calculate number of bytes to represent term as binary
static ssize_t bytesize_of_term(ERL_NIF_TERM term)
{
    if (PyInt_Check(term)) {
	long value = PyInt_AsLong(term);
	if ((value >= 0) && (value <= 0xff))
	    return 1+1;  // small_integer (uint8)
	else if ((value >= -0x80000000) && (value <= 0x7ffffff))
	    return 1+4;  // integer (int32)
	else // encode as bignum
	    return 1+1+1+4; // size,sign,byte*4
    }
    else if (PyLong_Check(term)) {
	size_t nbytes = (_PyLong_NumBits(term) + 7) / 8;
	// size,sign,byte*4
	return 1+1+1+nbytes;
    }    
    else if (PyBool_Check(term)) {
	if (term == Py_True)
	    return 1+1+4;  // (small atom)+length+4
	else
	    return 1+1+5;  // (small atom)+length+5
    }
    else if (PyFloat_Check(term)) {
	return 1+8;  // NEW_FLOAT
    }
    else if (PyString_Check(term)) {
	ssize_t len = PyString_Size(term);
	if (len <= 0xffff)
	    return 1+2+len;  // STRING encode
	else
	    return 1+4+2*len + 1; // LIST encode (with terminating nil)!
    }
    else if (PyList_Check(term)) {
	ssize_t len = PyList_Size(term);
	ssize_t size = 1+4 + 1;  // and the terminating NIL
	int i;
	for (i = 0; i < (int)len; i++) {
	    ssize_t size1;
	    if ((size1 = bytesize_of_term(PyList_GET_ITEM(term, i))) < 0)
		return -1;
	    size += size1;
	}
	return size;
    }    
    else if (PyTuple_Check(term)) {
	ssize_t len = PyTuple_Size(term);
	size_t size;
	int i;
	if (len <= 0xff)
	    size = 1+1;
	else
	    size = 1+4;
	for (i = 0; i < (int)len; i++) {
	    ssize_t size1;
	    if ((size1 = bytesize_of_term(PyTuple_GET_ITEM(term, i))) < 0)
		return -1;
	    size += size1;
	}
	return size;
    }
    else if (PyDict_Check(term)) {
	PyObject* key;
	PyObject* value;
	Py_ssize_t pos = 0;
	size_t size = 1+4;  // tag + length
	while(PyDict_Next(term, &pos, &key, &value)) {
	    ssize_t size1;
	    if ((size1 = bytesize_of_term(key)) < 0)
		return -1;
	    size += size1;
	    if ((size1 = bytesize_of_term(value)) < 0)
		return -1;
	    size += size1;
	}
	return size;
    }
    return -1;
}


#define DSUBb(a,b,r,d) do {						\
	uint8_t ___cr = (r);						\
	uint8_t ___xr = (a);						\
	uint8_t ___yr = (b)+___cr;					\
	___cr = (___yr < ___cr);					\
	___yr = ___xr - ___yr;						\
	___cr += (___yr > ___xr);					\
	d = ___yr;							\
	r = ___cr;							\
    } while(0)

#define DSUB(a,b,r,d) do {		\
	uint8_t ___xr = (a);	\
	uint8_t ___yr = (b);	\
	___yr = ___xr - ___yr;		\
	r = (___yr > ___xr);		\
	d = ___yr;			\
    } while(0)

// negate, two complement the bytes in src into x
static size_t negate_bytes(unsigned char* y, size_t yl, unsigned char* r)
{
    size_t rl = yl;
    unsigned char b, d;
    if (yl == 0)
	return 0;
    DSUB(*y,1,b,d);
    *r++ = ~d;
    y++;
    yl--;
    while(yl--) {
	DSUBb(*y,0,b,d);
	*r++ = ~d;
	y++;
    }
    if (b) {
	*r++ = 0xFF;
	return rl+1;
    }
    return rl;
}

static size_t decode_term(unsigned char* ptr, size_t len, PyObject** term);

static size_t decode_seq(unsigned char* ptr, size_t len,
			 PyObject** seq, size_t seqlen)
{
    int i;
    size_t blen = 0;
    for (i = 0; i < (int)seqlen; i++) {
	size_t ilen;
	if ((ilen = decode_term(ptr, len, &seq[i])) == 0)
	    return 0;
	blen += ilen;
	ptr += ilen;
	len -= ilen;
    }
    return blen;
}


static size_t decode_term(unsigned char* ptr, size_t len, PyObject** term)
{
    if (len < 1)
	return 0;
    DBG("decode_term: tag=%s\n", format_ext_tag(ptr[0]));
    switch(ptr[0]) {
    case NIL:
	if ((*term = PyList_New(0)) == NULL)
	    return 0;
	return 1;
    case SMALL_ATOM:
	return 0; // FIXME
    case ATOM:
	return 0; // FIXME	
    case BINARY: {
	size_t blen;
	if (len < 5) return 0;
	blen = get_uint32(ptr+1);
	DBG("decode_term: binary size=%ld\n", blen);
	if ((*term = PyByteArray_FromStringAndSize((const char*)ptr+5, blen)) == NULL)
	    return 0;
	return 1+4+blen;
    }
    case SMALL_INTEGER:
	if (len < 2) return 0;
	if ((*term = PyInt_FromLong(get_uint8(ptr+1))) == NULL)
	    return 0;
	return 1+1;
    case INTEGER:
	if (len < 5) return 0;
	if ((*term = PyInt_FromLong(get_int32(ptr+1))) == NULL)
	    return 0;
	return 1+4;
    case SMALL_BIG: { // t,n0,s,d0,d1,...dn-1
	size_t blen;
	if (len < 3) return 0;
	blen = get_uint8(ptr+1);
	DBG("decode_term: #digits=%ld, sign=%d\n", blen, ptr[2]);
	if (ptr[2]) { // sign, negate bytes
	    unsigned char digits[blen+1];
	    blen = negate_bytes(ptr+3, blen, digits);
	    if ((*term = _PyLong_FromByteArray(digits, blen, 1, 1)) == NULL)
		return 0;
	}
	else {
	    if ((*term = _PyLong_FromByteArray(ptr+3, blen, 1, 0)) == NULL)
		return 0;
	}
	return 1+1+1+blen;
    }
    case LARGE_BIG: {  // t,n3,n2,n1,n0,s,d0,d1,...dn-1
	size_t blen;
	if (len < 6) return 0;
	blen = get_uint32(ptr+1);
	DBG("decode_term: #digits=%ld, sign=%d\n", blen, ptr[5]);
	if (ptr[5]) { // sign, negate bytes
	    unsigned char digits[blen+1];
	    blen = negate_bytes(ptr, blen, digits);
	    if ((*term = _PyLong_FromByteArray(digits, blen, 1, 1)) == NULL)
		return 0;
	}	
	else {
	    if ((*term = _PyLong_FromByteArray(ptr+6, blen, 1, 0)) == NULL)
		return 0;
	}
	return 1+4+1+blen;
    }	
    case FLOAT: {  // t,string(31)
	char* junk = NULL;
	PyObject* string = PyString_FromStringAndSize((const char*)ptr+1, 31);
	if ((*term = PyFloat_FromString(string, &junk)) == NULL)
	    return 0;
	return 32;
    }
    case NEW_FLOAT: { // t,f7,f6,f5,f4,f3,f2,f1,f0
	double d;
	if (len < 9) return 0;
	d = _PyFloat_Unpack8(ptr+1, 0);
	if ((*term = PyFloat_FromDouble(d)) == NULL)
	    return 0;
	return 1+8;
    }
    case REFERENCE:
	return 0; // FIXME
    case NEW_REFERENCE:
	return 0; // FIXME
    case NEWER_REFERENCE:
	return 0; // FIXME
    case STRING: {
	size_t blen;
	if (len < 3) return 0;
	blen = get_uint16(ptr+1);
	DBG("decode_term: string length=%ld\n", blen);
	if ((*term = PyString_FromStringAndSize((const char*)ptr+3, blen)) == NULL)
	    return 0;
	return 1+2+blen;
    }
    case LIST: {
	size_t n;
	if (len < 5) return 0;
	n = get_uint32(ptr+1);
	DBG("decode_term: list length=%ld\n", n);
	{
	    PyObject* list;
	    PyObject* seq[n];
	    size_t slen;
	    int i;
	    
	    if ((slen = decode_seq(ptr+5, len-5, seq, n)) == 0)
		return 0;
	    if (len-5-slen < 1)
		return 0;
	    if (ptr[5+slen] != NIL)
		return 0;
	    if ((list = PyList_New(n)) == NULL)
		return 0;
	    for (i = 0; i < (int)n; i++)
		PyList_SetItem(list, i, seq[i]);
	    *term = list;	    
	    return 5+slen+1;
	}
    }
    case SMALL_TUPLE: {
	size_t n;
	if (len < 2) return 0;
	n = get_uint8(ptr+1);
	DBG("decode_term: tuple length=%ld\n", n);
	{
	    PyObject* tuple;
	    PyObject* seq[n];
	    size_t slen;
	    int i;
	    
	    if ((slen = decode_seq(ptr+2, len-2, seq, n)) == 0)
		return 0;
	    if ((tuple = PyTuple_New(n)) == NULL)
		return 0;
	    for (i = 0; i < (int)n; i++)
		PyTuple_SetItem(tuple, i, seq[i]);
	    *term = tuple;
	    return 2+slen;
	}
    }
    case LARGE_TUPLE: {
	size_t n;
	if (len < 5) return 0;
	n = get_uint32(ptr+1);
	DBG("decode_term: tuple length=%ld\n", n);
	{
	    PyObject* tuple;
	    PyObject* seq[n];
	    size_t slen;
	    int i;
	    
	    if ((slen = decode_seq(ptr+5, len-5, seq, n)) == 0)
		return 0;
	    if ((tuple = PyTuple_New(n)) == NULL)
		return 0;
	    for (i = 0; i < (int)n; i++)
		PyTuple_SetItem(tuple, i, seq[i]);
	    *term = tuple;
	    return 2+slen;
	}
    }
    case MAP: {
	size_t n;
	if (len < 5) return 0;
	n = get_uint32(ptr+1);
	DBG("decode_term: map length=%ld\n", n);
	{
	    PyObject* dict;
	    PyObject* seq[2*n];
	    size_t slen;
	    int i;
	    
	    if ((slen = decode_seq(ptr+5, len-5, seq, 2*n)) == 0)
		return 0;
	    if ((dict = PyDict_New()) == NULL)
		return 0;
	    for (i = 0; i < (int)n; i += 2)
		PyDict_SetItem(dict, seq[i], seq[i+1]);
	    *term = dict;
	    return 5+slen;
	}
    }
    default:
	return 0;
    }
}

ssize_t encode_term(PyObject* term, unsigned char* ptr, ssize_t size)
{
    if (PyInt_Check(term)) {
	long value = PyInt_AsLong(term);
	if ((value >= 0) && (value <= 0xff)) {
	    if (size < 2) return -1;
	    DBG("encode_term: SMALL_INTEGER %ld\n", value);
	    ptr[0] = SMALL_INTEGER;
	    put_uint8(ptr+1, value);
	    return 1+1;
	}
	else if ((value >= -0x80000000) && (value <= 0x7ffffff)) {
	    if (size < 5) return -1;
	    DBG("encode_term: INTEGER %ld\n", value);
	    ptr[0] = INTEGER;
	    put_int32(ptr+1, value);
	    return 1+4;
	}
	else { // encode as (small) bignum
	    if (value < 0) {
		ssize_t len = bytesize_of_ulong(-value);
		if (size < len+3) return -1;
		DBG("encode_term: SMALL_BIG size=%ld\n", len);
		ptr[0] = SMALL_BIG;
		ptr[1] = len;
		ptr[2] = 1;
		encode_ulong(ptr+3, -value);
		return 3+len;
	    }
	    else {
		ssize_t len = bytesize_of_ulong(value);
		if (size < len+3) return -1;
		DBG("encode_term: SMALL_BIG size=%ld\n", len);
		ptr[0] = SMALL_BIG;
		ptr[1] = len;
		ptr[2] = 0;
		encode_ulong(ptr+3, value);
		return 3+len;
	    }
	}
    }
    else if (PyLong_Check(term)) {
	size_t nbytes = (_PyLong_NumBits(term) + 7) / 8;
	int is_signed = _PyLong_Sign(term);
	if (nbytes <= 255) {
	    if (size < (ssize_t)nbytes+3) return -1;
	    DBG("encode_term: SMALL_BIG size=%ld\n", nbytes);
	    ptr[0] = SMALL_BIG;
	    put_uint8(ptr+1, nbytes);
	    ptr[2] = is_signed;
	    _PyLong_AsByteArray((PyLongObject*)term,
				ptr+3, nbytes, 1, is_signed);
	    if (is_signed)
		nbytes = negate_bytes(ptr+3, nbytes, ptr+3);
	    return 1+1+1+nbytes;	    
	}
	else {
	    if (size < (ssize_t)nbytes+6) return -1;
	    DBG("encode_term: LARGE_BIG %ld\n", nbytes);
	    ptr[0] = LARGE_BIG;
	    put_uint32(ptr+1, nbytes);
	    ptr[5] = is_signed;
	    _PyLong_AsByteArray((PyLongObject*)term,
				ptr+6, nbytes, 1, is_signed);
	    if (is_signed)
		nbytes = negate_bytes(ptr+6, nbytes, ptr+6);
	    return 1+4+1+nbytes;
	}
    }
    else if (PyBool_Check(term)) {
	if (term == Py_True) {
	    if (size < 6) return -1;
	    DBG("encode_term: SMALL_ATOM %d true\n", 4);
	    ptr[0]=SMALL_ATOM;
	    ptr[1]=4;
	    ptr[2]='t'; ptr[3]='r'; ptr[4]='u'; ptr[5]='e';
	    return 1+1+4;  // (small atom)+length+4
	}
	else {
	    if (size < 7) return -1;
	    DBG("encode_term: SMALL_ATOM %d false\n", 5);
	    ptr[0]=SMALL_ATOM;
	    ptr[1]=5;
	    ptr[2]='f'; ptr[3]='a'; ptr[4]='l'; ptr[5]='s'; ptr[6]='e';
	    return 1+1+5;  // (small atom)+length+5
	}
    }
    else if (PyFloat_Check(term)) {
	if (size < 9) return -1;
	DBG("encode_term: NEW_FLOAT %d\n", 8);
	ptr[0] = NEW_FLOAT;
	_PyFloat_Pack8(PyFloat_AsDouble(term), ptr+1, 0);
	return 1+8;  // NEW_FLOAT
    }
    else if (PyString_Check(term)) {
	char* str;
	Py_ssize_t len;
	if (PyString_AsStringAndSize(term, &str, &len) < 0)
	    return 0;
	if (len <= 0xffff) {
	    if (size < 3+len) return -1;
	    DBG("encode_term: STRING length=%ld\n", len);
	    ptr[0] = STRING;
	    put_uint16(ptr+1, len);
	    memcpy(ptr+3, str, len);
	    return 1+2+len;  // STRING encode
	}
	else {
	    int n = (int)len;
	    if (size < 5+2*len+1) return -1;
	    DBG("encode_term: LIST length=%ld\n", len);
	    ptr[0] = LIST;
	    put_uint32(ptr+1, len);
	    ptr += 5;
	    while(n--) {
		*ptr++ = SMALL_INTEGER;
		*ptr++ = *str++;
	    }
	    *ptr = NIL;
	    return 1+4+2*len + 1; // LIST encode (with terminating nil)!
	}
    }
    else if (PyList_Check(term)) {
	ssize_t len = PyList_Size(term);
	ssize_t bsize = 1+4+1;  // and the terminating NIL
	int i;
	if (size < 6) return -1;
	DBG("encode_term: LIST length=%ld\n", len);
	ptr[0] = LIST;
	put_uint32(ptr+1, len);
	ptr += 5;
	size -= 5;
	for (i = 0; i < (int)len; i++) {
	    ssize_t size1;
	    if ((size1 = encode_term(PyList_GET_ITEM(term, i),ptr,size))<=0)
		return -1;
	    ptr += size1;
	    size -= size1;
	    bsize += size1;
	}
	*ptr = NIL;
	return bsize;
    }
    else if (PyTuple_Check(term)) {
	ssize_t len = PyTuple_Size(term);
	ssize_t bsize;
	int i;
	if (len <= 0xff) {
	    if (size < 2) return -1;
	    DBG("encode_term: SMALL_TUPLE size=%ld\n", len);
	    bsize = 1+1;
	    ptr[0] = SMALL_TUPLE;
	    put_uint8(ptr+1, len);
	    ptr += 2;
	    size -= 2;
	}
	else {
	    if (size < 5) return -1;
	    DBG("encode_term: LARGE_TUPLE size=%ld\n", len);
	    bsize = 1+4;
	    ptr[0] = LARGE_TUPLE;
	    put_uint32(ptr+1, len);
	    ptr += 5;
	    size -= 5;
	}
	for (i = 0; i < (int)len; i++) {
	    ssize_t size1;
	    if ((size1=encode_term(PyTuple_GET_ITEM(term, i),ptr,size))<=0)
		return -1;
	    ptr += size1;
	    size -= size1;
	    bsize += size1;
	}
	return bsize;
    }
    else if (PyDict_Check(term)) {
	PyObject* key;
	PyObject* value;
	ssize_t len = PyDict_Size(term);
	Py_ssize_t pos = 0;
	ssize_t bsize = 1+4;  // tag + length

	if (size < 5) return -1;
	DBG("encode_term: MAP #entries=%ld\n", len);
	bsize = 1+4;
	ptr[0] = MAP;
	put_uint32(ptr+1, len);
	ptr += 5;
	size -= 5;	
	
	while(PyDict_Next(term, &pos, &key, &value)) {
	    ssize_t size1;
	    if ((size1=encode_term(key,ptr,size))<=0)
		return -1;
	    ptr += size1;
	    size -= size1;
	    bsize += size1;
	    if ((size1=encode_term(value,ptr,size)) <= 0)
		return -1;
	    ptr += size1;
	    size -= size1;
	    bsize += size1;	    
	}
	return bsize;
    }
    return -1;
}
    
int enif_term_to_binary(ErlNifEnv *env, ERL_NIF_TERM term, ErlNifBinary *bin)
{
    ssize_t size;
    if ((size = bytesize_of_term(term)) <= 0)
	return 0;
    DBG("term_to_binary: size=%ld\n", size);
    if (!enif_alloc_binary(size+1, bin))
	return 0;
    if (encode_term(term, bin->data+1, size) <= 0) {
	enif_release_binary(bin);
	return 0;
    }
    bin->data[0] = MAGIC;
    return 1;
}

size_t enif_binary_to_term(ErlNifEnv *env, const unsigned char* data,
			   size_t sz, ERL_NIF_TERM *term, unsigned int opts)
{
    unsigned char* ptr = (unsigned char*) data;
    size_t size;
    if (ptr[0] != MAGIC)
	return 0;
    if ((size = decode_term(ptr+1, sz-1, term)) == 0)
	return 0;
    DBG("binary_to_term: size=%ld\n", size);
    return size+1;
}

//
// Modules
// if STATIC_ERLANG_NIF was set at build time then the init function
// is call <modname>_nif_init(ERL_NIF_INIT_ARGS)
//
static ErlNifEntry* nif_entry = NULL;
static ErlNifEnv nif_env;
// static int min_arity[MAX_PYNIF_FUNCS];
// static int max_arity[MAX_PYNIF_FUNCS];
static int fun_start[MAX_PYNIF_FUNCS];
static int fun_end[MAX_PYNIF_FUNCS];
static int nif_ari[MAX_PYNIF_FUNCS];
static int nif_fun[MAX_PYNIF_FUNCS];

static PyObject* pynif_call(PyObject* self, PyObject* args, int j)
{
    PyObject** argv = ((PyTupleObject *)(args))->ob_item;
    int argc = PyTuple_Size(args);
    int i;
    
    nif_env.self = self;

    for (i = fun_start[j]; i < fun_end[j]; i++) {
	if (nif_ari[i] == argc) {
	    int k = nif_fun[i];
	    return (*nif_entry->funcs[k].fptr)(&nif_env, argc, argv);
	}
    }
    PyErr_SetString(PyExc_TypeError, "badarity");
    fprintf(stderr, "arity mismatch %s got %d args\r\n",
	    nif_entry->funcs[i].name, argc);
    return NULL;
}

#define PF(i) static PyObject* CAT2(pf,i)(PyObject* self, PyObject* args) { return pynif_call(self, args, (i)); }

// must be a way? ... max 256 right now
PF(0) PF(1) PF(2) PF(3) PF(4) PF(5) PF(6) PF(7) 
PF(8) PF(9) PF(10) PF(11) PF(12) PF(13) PF(14) PF(15) 
PF(16) PF(17) PF(18) PF(19) PF(20) PF(21) PF(22) PF(23) 
PF(24) PF(25) PF(26) PF(27) PF(28) PF(29) PF(30) PF(31) 
PF(32) PF(33) PF(34) PF(35) PF(36) PF(37) PF(38) PF(39) 
PF(40) PF(41) PF(42) PF(43) PF(44) PF(45) PF(46) PF(47) 
PF(48) PF(49) PF(50) PF(51) PF(52) PF(53) PF(54) PF(55) 
PF(56) PF(57) PF(58) PF(59) PF(60) PF(61) PF(62) PF(63) 
PF(64) PF(65) PF(66) PF(67) PF(68) PF(69) PF(70) PF(71) 
PF(72) PF(73) PF(74) PF(75) PF(76) PF(77) PF(78) PF(79) 
PF(80) PF(81) PF(82) PF(83) PF(84) PF(85) PF(86) PF(87) 
PF(88) PF(89) PF(90) PF(91) PF(92) PF(93) PF(94) PF(95) 
PF(96) PF(97) PF(98) PF(99) PF(100) PF(101) PF(102) PF(103) 
PF(104) PF(105) PF(106) PF(107) PF(108) PF(109) PF(110) PF(111) 
PF(112) PF(113) PF(114) PF(115) PF(116) PF(117) PF(118) PF(119) 
PF(120) PF(121) PF(122) PF(123) PF(124) PF(125) PF(126) PF(127) 
PF(128) PF(129) PF(130) PF(131) PF(132) PF(133) PF(134) PF(135) 
PF(136) PF(137) PF(138) PF(139) PF(140) PF(141) PF(142) PF(143) 
PF(144) PF(145) PF(146) PF(147) PF(148) PF(149) PF(150) PF(151) 
PF(152) PF(153) PF(154) PF(155) PF(156) PF(157) PF(158) PF(159) 
PF(160) PF(161) PF(162) PF(163) PF(164) PF(165) PF(166) PF(167) 
PF(168) PF(169) PF(170) PF(171) PF(172) PF(173) PF(174) PF(175) 
PF(176) PF(177) PF(178) PF(179) PF(180) PF(181) PF(182) PF(183) 
PF(184) PF(185) PF(186) PF(187) PF(188) PF(189) PF(190) PF(191) 
PF(192) PF(193) PF(194) PF(195) PF(196) PF(197) PF(198) PF(199) 
PF(200) PF(201) PF(202) PF(203) PF(204) PF(205) PF(206) PF(207) 
PF(208) PF(209) PF(210) PF(211) PF(212) PF(213) PF(214) PF(215) 
PF(216) PF(217) PF(218) PF(219) PF(220) PF(221) PF(222) PF(223) 
PF(224) PF(225) PF(226) PF(227) PF(228) PF(229) PF(230) PF(231) 
PF(232) PF(233) PF(234) PF(235) PF(236) PF(237) PF(238) PF(239) 
PF(240) PF(241) PF(242) PF(243) PF(244) PF(245) PF(246) PF(247) 
PF(248) PF(249) PF(250) PF(251) PF(252) PF(253) PF(254) PF(255)


static PyCFunction pynif_func[MAX_PYNIF_FUNCS] =
{
pf0,pf1,pf2,pf3,pf4,pf5,pf6,pf7,
pf8,pf9,pf10,pf11,pf12,pf13,pf14,pf15,
pf16,pf17,pf18,pf19,pf20,pf21,pf22,pf23,
pf24,pf25,pf26,pf27,pf28,pf29,pf30,pf31,
pf32,pf33,pf34,pf35,pf36,pf37,pf38,pf39,
pf40,pf41,pf42,pf43,pf44,pf45,pf46,pf47,
pf48,pf49,pf50,pf51,pf52,pf53,pf54,pf55,
pf56,pf57,pf58,pf59,pf60,pf61,pf62,pf63,
pf64,pf65,pf66,pf67,pf68,pf69,pf70,pf71,
pf72,pf73,pf74,pf75,pf76,pf77,pf78,pf79,
pf80,pf81,pf82,pf83,pf84,pf85,pf86,pf87,
pf88,pf89,pf90,pf91,pf92,pf93,pf94,pf95,
pf96,pf97,pf98,pf99,pf100,pf101,pf102,pf103,
pf104,pf105,pf106,pf107,pf108,pf109,pf110,pf111,
pf112,pf113,pf114,pf115,pf116,pf117,pf118,pf119,
pf120,pf121,pf122,pf123,pf124,pf125,pf126,pf127,
pf128,pf129,pf130,pf131,pf132,pf133,pf134,pf135,
pf136,pf137,pf138,pf139,pf140,pf141,pf142,pf143,
pf144,pf145,pf146,pf147,pf148,pf149,pf150,pf151,
pf152,pf153,pf154,pf155,pf156,pf157,pf158,pf159,
pf160,pf161,pf162,pf163,pf164,pf165,pf166,pf167,
pf168,pf169,pf170,pf171,pf172,pf173,pf174,pf175,
pf176,pf177,pf178,pf179,pf180,pf181,pf182,pf183,
pf184,pf185,pf186,pf187,pf188,pf189,pf190,pf191,
pf192,pf193,pf194,pf195,pf196,pf197,pf198,pf199,
pf200,pf201,pf202,pf203,pf204,pf205,pf206,pf207,
pf208,pf209,pf210,pf211,pf212,pf213,pf214,pf215,
pf216,pf217,pf218,pf219,pf220,pf221,pf222,pf223,
pf224,pf225,pf226,pf227,pf228,pf229,pf230,pf231,
pf232,pf233,pf234,pf235,pf236,pf237,pf238,pf239,
pf240,pf241,pf242,pf243,pf244,pf245,pf246,pf247,
pf248,pf249,pf250,pf251,pf252,pf253,pf254,pf255,
};


#ifdef WIN32
#define RTLD_LAZY 0
typedef HMODULE dl_handle_t;
void * dlsym(HMODULE Lib, const char *func) {
    return (void *) GetProcAddress(Lib, func);
}
HMODULE dlopen(const CHAR *DLL, int unused) {
  UNUSED(unused);
  return LoadLibrary(DLL);
}
#else
typedef void * dl_handle_t;
#endif

// #ifndef PYNIFFILE
// extern ErlNifEntry* nif_init(void);
// #endif

static int is_method_installed(PyMethodDef* methods, size_t num_methods,
			       const char* name)
{
    size_t j;
    for (j = 0; j < num_methods; j++) {
	if (strcmp(name, methods[j].ml_name) == 0)
	    return 1;
    }
    return 0;
}

PyMODINIT_FUNC
CAT2(init,PYNIFNAME)(void)
{
    // now convert all funcs into PyMethodDef array
    PyObject *m;
    PyMethodDef* methods;
    int i;
    int fi;
    size_t num_methods;
    
#ifdef PYNIFFILE
    ErlNifEntry* (*nif_init)(void);
#else
    extern ErlNifEntry* nif_init(void);
#endif

#ifdef PYNIFFILE
    {
	dl_handle_t handle;
	char* file = STRINGIFY(PYNIFFILE);
	// load dynamic library
	if ((handle = dlopen(file,RTLD_LAZY)) == NULL) {
	    fprintf(stderr, "Failed open %s dynamic library\r\n", file);
	    // FIXME: set error
	    return;
	}
	// first find the proc addr function
	nif_init = (ErlNifEntry* (*)(void)) dlsym(handle,"nif_init");
    }
#endif
    
    nif_entry = nif_init();

//  fprintf(stderr, "nif_entry: name = %s\n", nif_entry->name);
//  fprintf(stderr, "nif_entry: num_of_funcs = %d\n", nif_entry->num_of_funcs);

    if (nif_entry->num_of_funcs > MAX_PYNIF_FUNCS) {
	fprintf(stderr, "sorry to many functions limit is %d \r\n",
		MAX_PYNIF_FUNCS);
	fprintf(stderr, "  to fix update MAX_PYNIF_FUNS!\r\n");
	return;
    }
    
    methods = malloc(sizeof(PyMethodDef)*(nif_entry->num_of_funcs+1));
    memset(methods, 0, sizeof(PyMethodDef)*(nif_entry->num_of_funcs+1));
    
    fi = 0;
    num_methods = 0;
    
    for (i = 0; i < nif_entry->num_of_funcs; i++) {
	const char* name = nif_entry->funcs[i].name;
	int arity;
	int j, k;
	
	if (is_method_installed(methods, num_methods, name))
	    continue;
	j = num_methods++;
	methods[j].ml_name = name;
	methods[j].ml_meth = pynif_func[j];
	methods[j].ml_flags =  METH_VARARGS;
	methods[j].ml_doc = "PyNif function";

	arity = nif_entry->funcs[i].arity;
	// min_arity[j] = arity;
	// max_arity[j] = arity;
	fun_start[j] = fi; // entry start for arity/nif_entry pair
	nif_ari[fi] = arity;
	nif_fun[fi] = i;
	fi++;
	fun_end[j] = fi;

	// install all function with same name
	for (k = i+1; k < nif_entry->num_of_funcs; k++) {
	    if (strcmp(name, nif_entry->funcs[k].name) == 0) {
		arity = nif_entry->funcs[k].arity;
		// min_arity[j] = min(min_arity[j], arity);
		// max_arity[j] = max(min_arity[j], arity);
		nif_ari[fi]  = arity;
		nif_fun[fi]  = k;
		fi++;
		fun_end[j] = fi;
	    }
	}
    }

    m = Py_InitModule(STRINGIFY(PYNIFNAME), methods);

    memset(&nif_env, 0, sizeof(ErlNifEnv));

    nif_env.atoms = PyDict_New();
    nif_env.atom_table_size = INITIAL_ATOM_TABLE_SIZE;
    nif_env.atom_table = PyMem_Malloc(INITIAL_ATOM_TABLE_SIZE*sizeof(PyObject*));
    PyModule_AddIntConstant(m, "false", 0);
    PyDict_SetItemString(nif_env.atoms, "false", Py_False);
    nif_env.atom_table[0] = Py_False;
    
    PyModule_AddIntConstant(m, "true", 1);
    PyDict_SetItemString(nif_env.atoms, "true", Py_True);
    nif_env.atom_table[1] = Py_True;

    nif_env.atom_index = 2;
    
    nif_env.module = m;
    nif_env.self   = m;

    if (nif_entry->load != NULL) {
	ERL_NIF_TERM load_info = enif_make_int(&nif_env, 0);
	int r = nif_entry->load(&nif_env, &nif_env.priv_data, load_info);
	if (r < 0) return;
    }
}

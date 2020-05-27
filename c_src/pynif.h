//
//  pynif: enif wrapper to allow python to call erlang nifs
//
#ifndef __PYNIF_H__
#define __PYNIF_H__

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <pythread.h>

typedef PyObject* ERL_NIF_TERM;
typedef struct _erl_nif_mutex ErlNifMutex;
typedef struct _erl_nif_rwlock ErlNifRWLock;
typedef PyTypeObject ErlNifResourceType;
typedef int ErlNifTSDKey;

typedef struct enif_env_t
{
    ERL_NIF_TERM self;
    void* priv_data;
    ERL_NIF_TERM module;    // PyModule object
    PyObject* atoms;        // map: string => string
    size_t atom_table_size; // number of allocated items in atom_table
    PyObject** atom_table;  // array: atom_index => bool|string_object
    size_t atom_index;
} ErlNifEnv;

#define INITIAL_ATOM_TABLE_SIZE 1024
#define EXPAND_ATOM_TABLE_SIZE  512

typedef enum
{
    ERL_NIF_RT_CREATE = 1,
    ERL_NIF_RT_TAKEOVER = 2
} ErlNifResourceFlags;

typedef enum
{
    ERL_NIF_LATIN1 = 1
} ErlNifCharEncoding;

typedef struct
{
    size_t size;
    unsigned char* data;
    
    void* ref_bin;
    /* for future additions to be ABI compatible (same struct size) */
    void* __spare__[2];
} ErlNifBinary;

typedef struct enif_func_t
{
    const char* name;
    unsigned arity;
    ERL_NIF_TERM (*fptr)(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]);
    unsigned flags;
} ErlNifFunc;

typedef struct enif_entry_t
{
    int major;
    int minor;
    const char* name;
    int num_of_funcs;
    ErlNifFunc* funcs;
    int  (*load)   (ErlNifEnv*, void** priv_data, ERL_NIF_TERM load_info);
    int  (*reload) (ErlNifEnv*, void** priv_data, ERL_NIF_TERM load_info);
    int  (*upgrade)(ErlNifEnv*, void** priv_data, void** old_priv_data, ERL_NIF_TERM load_info);
    void (*unload) (ErlNifEnv*, void* priv_data);

    /* Added in 2.1 */
    const char* vm_variant;

    /* Added in 2.7 */
    unsigned options;   /* Unused. Can be set to 0 or 1 (dirty sched config) */

    /* Added in 2.12 */
    size_t sizeof_ErlNifResourceTypeInit;

    /* Added in 2.14 */
    const char* min_erts;
} ErlNifEntry;

#endif

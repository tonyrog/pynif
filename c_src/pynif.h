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

#if (PY_MAJOR_VERSION > 3) || ((PY_MAJOR_VERSION==3) && (PY_MINOR_VERSION>=7))
#define USE_TSS_KEY 1
typedef Py_tss_t ErlNifTSDKey;
#else
#undef USE_TSS_KEY
typedef int ErlNifTSDKey;
#endif

#define SMALL_INTEGER_EXT 'a'
#define INTEGER_EXT       'b'
#define FLOAT_EXT         'c'
#define ATOM_EXT          'd'
#define SMALL_ATOM_EXT    's'
#define REFERENCE_EXT     'e'
#define NEW_REFERENCE_EXT 'r'
#define NEWER_REFERENCE_EXT 'Z'
#define PORT_EXT          'f'
#define NEW_PORT_EXT      'Y'
#define NEW_FLOAT_EXT     'F'
#define PID_EXT           'g'
#define NEW_PID_EXT       'X'
#define SMALL_TUPLE_EXT   'h'
#define LARGE_TUPLE_EXT   'i'
#define NIL_EXT           'j'
#define STRING_EXT        'k'
#define LIST_EXT          'l'
#define BINARY_EXT        'm'
#define BIT_BINARY_EXT    'M'
#define SMALL_BIG_EXT     'n'
#define LARGE_BIG_EXT     'o'
#define NEW_FUN_EXT       'p'
#define EXPORT_EXT        'q'
#define MAP_EXT           't'
#define FUN_EXT           'u'
#define ATOM_UTF8_EXT     'v'
#define SMALL_ATOM_UTF8_EXT 'w'

#define VERSION_MAGIC 131

typedef struct enif_env_t
{
    ERL_NIF_TERM self;
    void* priv_data;
    ERL_NIF_TERM module;    // PyModule object
    PyObject* atoms;        // map: string => string
    size_t atom_table_size; // number of allocated items in atom_table
    PyObject** atom_table;  // array: atom_index => bool|string_object
    size_t atom_index;
    PyObject* autodispose_list; // list: of objects to remove
} ErlNifEnv;

#define INITIAL_ATOM_TABLE_SIZE 1024
#define EXPAND_ATOM_TABLE_SIZE  512

#if (defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_))
typedef void* ErlNifEvent; /* FIXME: Use 'HANDLE' somehow without breaking existing source */
#else
typedef int ErlNifEvent;
#endif

/* Return bits from enif_select: */
#define ERL_NIF_SELECT_STOP_CALLED    (1 << 0)
#define ERL_NIF_SELECT_STOP_SCHEDULED (1 << 1)
#define ERL_NIF_SELECT_INVALID_EVENT  (1 << 2)
#define ERL_NIF_SELECT_FAILED         (1 << 3)
#define ERL_NIF_SELECT_READ_CANCELLED (1 << 4)
#define ERL_NIF_SELECT_WRITE_CANCELLED (1 << 5)

typedef enum
{
    ERL_NIF_RT_CREATE = 1,
    ERL_NIF_RT_TAKEOVER = 2
} ErlNifResourceFlags;

typedef struct
{
    long thread_ident;
} ErlNifPid;

typedef struct {
    unsigned char data[sizeof(void *)*4];
} ErlNifMonitor;

typedef void ErlNifResourceDtor(ErlNifEnv*, void*);
typedef void ErlNifResourceStop(ErlNifEnv*, void*, ErlNifEvent, int is_direct_call);
typedef void ErlNifResourceDown(ErlNifEnv*, void*, ErlNifPid*, ErlNifMonitor*);

typedef struct {
    ErlNifResourceDtor* dtor;
    ErlNifResourceStop* stop;  /* at ERL_NIF_SELECT_STOP event */
    ErlNifResourceDown* down;  /* enif_monitor_process */
} ErlNifResourceTypeInit;


typedef enum
{
    ERL_NIF_LATIN1 = 1
} ErlNifCharEncoding;

typedef struct
{
    size_t size;
    unsigned char* data;
    PyObject* ref_bin;
    int mutable;
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

//
// A standalone module access from a driver
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "erl_nif.h"

typedef struct demo2_state_t
{
    int value;
} demo2_state_t;

// Atom macros
#define ATOM(name) atm_##name

#define DECL_ATOM(name) \
    ERL_NIF_TERM atm_##name = 0

// require env in context (ugly)
#define LOAD_ATOM(name)			\
    atm_##name = enif_make_atom(env,#name)

#define LOAD_ATOM_STRING(name,string)			\
    atm_##name = enif_make_atom(env,string)

#define UNUSED(var) (void)var

// General atoms
DECL_ATOM(ok);
DECL_ATOM(error);
DECL_ATOM(unknown);
DECL_ATOM(undefined);
DECL_ATOM(true);
DECL_ATOM(false);

static ERL_NIF_TERM next(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[]);

ErlNifFunc demo2_funcs[] = {
    { "next",    1, next    },
    { "next",    2, next    },
};

static ERL_NIF_TERM next(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[])
{
    if (argc == 1) {
	return enif_make_tuple2(env, ATOM(ok), enif_make_long(env, 1000));
    }
    else if (argc == 2) {
	return enif_make_tuple4(env, ATOM(true),
				enif_make_atom(env, "foo"),
				argv[0],
				argv[1]);
    }
    return enif_make_badarg(env);
}

static int  demo2_load(ErlNifEnv* env, void** priv_data, ERL_NIF_TERM load_info)
{
    UNUSED(env);
    demo2_state_t* state;

    fprintf(stderr, "demo2_load called\n");
    
    if (!(state = enif_alloc(sizeof(demo2_state_t))))
	return -1;
    state->value = 17;

    // General atoms
    LOAD_ATOM(ok);
    LOAD_ATOM(error);
    LOAD_ATOM(unknown);
    LOAD_ATOM(undefined);
    LOAD_ATOM(true);
    LOAD_ATOM(false);
    
    *priv_data = state;
    
    return 0;
}

static int demo2_upgrade(ErlNifEnv* env, void** priv_data, void** old_priv_data,
			 ERL_NIF_TERM load_info)
{
    UNUSED(env);
    fprintf(stderr, "demo2_upgrade called\n");
    *priv_data = *old_priv_data;
    return 0;
}

static void demo2_unload(ErlNifEnv* env, void* priv_data)
{
    UNUSED(env);
    demo2_state_t* state = priv_data;
    
    fprintf(stderr, "demo2_unload called\n");

    enif_free(state);
}


ERL_NIF_INIT(demo2, demo2_funcs,
	     demo2_load,
	     NULL,
	     demo2_upgrade,
	     demo2_unload)

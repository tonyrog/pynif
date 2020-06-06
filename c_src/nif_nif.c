//
// Test functions
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "erl_nif.h"

// Dirty optional since 2.7 and mandatory since 2.12
#if (ERL_NIF_MAJOR_VERSION > 2) || ((ERL_NIF_MAJOR_VERSION == 2) && (ERL_NIF_MINOR_VERSION >= 7))
#ifdef USE_DIRTY_SCHEDULER
#define NIF_FUNC(name,arity,fptr) {(name),(arity),(fptr),(ERL_NIF_DIRTY_JOB_CPU_BOUND)}
#define NIF_DIRTY_FUNC(name,arity,fptr) {(name),(arity),(fptr),(ERL_NIF_DIRTY_JOB_CPU_BOUND)}
#else
#define NIF_FUNC(name,arity,fptr) {(name),(arity),(fptr),(0)}
#define NIF_DIRTY_FUNC(name,arity,fptr) {(name),(arity),(fptr),(ERL_NIF_DIRTY_JOB_CPU_BOUND)}
#endif
#else
#define NIF_FUNC(name,arity,fptr) {(name),(arity),(fptr)}
#define NIF_DIRTY_FUNC(name,arity,fptr) {(name),(arity),(fptr)}
#endif


static ERL_NIF_TERM term_to_binary(ErlNifEnv* env,int argc,
				   const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM binary_to_term(ErlNifEnv* env,int argc,
				   const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM iolist_to_binary(ErlNifEnv* env,int argc,
				     const ERL_NIF_TERM argv[]);

ErlNifFunc nif_funcs[] = {
    NIF_FUNC( "term_to_binary",   1, term_to_binary  ),
    NIF_FUNC( "binary_to_term",   1, binary_to_term  ),
    NIF_FUNC( "iolist_to_binary",  1, iolist_to_binary ),
};


static ERL_NIF_TERM term_to_binary(ErlNifEnv* env,int argc,
				   const ERL_NIF_TERM argv[])
{
    ErlNifBinary bin;

    if (!enif_term_to_binary(env, argv[0], &bin))
	return enif_make_badarg(env);
    else
	return enif_make_binary(env, &bin);
}

static ERL_NIF_TERM binary_to_term(ErlNifEnv* env,int argc,
				   const ERL_NIF_TERM argv[])
{
    ErlNifBinary bin;

    if (!enif_inspect_iolist_as_binary(env, argv[0], &bin))
	return enif_make_badarg(env);
    else {
	ERL_NIF_TERM term;
	size_t n;
	if ((n = enif_binary_to_term(env, bin.data, bin.size,
				     &term, 0)) != bin.size)
	    return enif_make_badarg(env);
	return term;
    }
}

static ERL_NIF_TERM iolist_to_binary(ErlNifEnv* env,int argc,
				     const ERL_NIF_TERM argv[])
{
    ErlNifBinary bin;

    if (!enif_inspect_iolist_as_binary(env, argv[0], &bin))
	return enif_make_badarg(env);
    else
	return enif_make_binary(env, &bin);
}


ERL_NIF_INIT(nif, nif_funcs,
             0, 0, 0, 0)

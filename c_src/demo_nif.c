//
// A standalone module access from a driver
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

#define UNUSED(var) (void)var

static ERL_NIF_TERM info(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM hello(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM goodbye(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM echo(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM reverse(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[]);

ErlNifFunc demo_funcs[] = {
    NIF_FUNC( "info",    0, info    ),
    NIF_FUNC( "hello",   1, hello   ),
    NIF_FUNC( "goodbye", 1, goodbye ),
    NIF_FUNC( "hello",   2, hello   ),
    NIF_FUNC( "echo",    1, echo    ),
    NIF_FUNC( "reverse", 1, reverse )
};

static ERL_NIF_TERM info(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[])
{
    UNUSED(argc);
    UNUSED(argv);
    enif_fprintf(stdout, "info: demo funcs=%p\r\n", demo_funcs);
    enif_fprintf(stdout, "info: info=%p\r\n", demo_funcs[0].fptr);
    enif_fprintf(stdout, "info: env=%p\r\n", env);
    return enif_make_int(env, 0);
}

static ERL_NIF_TERM hello(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[])
{
    enif_fprintf(stderr, "hello argc=%d\r\n", argc);
    if (argc == 1) {
	long l;
	if (enif_get_long(env, argv[0], &l))
	    return enif_make_long(env, l + 1);
    }
    else if (argc == 2) {
	long l1, l2;
	if (enif_get_long(env, argv[0], &l1) &&
	    enif_get_long(env, argv[1], &l2))
	    return enif_make_long(env, l1 + l2 + 1);
    }
    return enif_make_badarg(env);
}


static ERL_NIF_TERM goodbye(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[])
{
    enif_fprintf(stderr, "goodbye argc=%d\r\n", argc);
    if (enif_is_atom(env, argv[0])) {
	char string[256];
	int i, j, len;
	enif_get_atom(env, argv[0], string, sizeof(string), ERL_NIF_LATIN1);
	len = strlen(string);
	i = 0;
	j = len-1;
	while(i < j) {
	    char t = string[i];
	    string[i] = string[j];
	    string[j] = t;
	    i++;
	    j--;
	}
	return enif_make_atom_len(env, string, len);
    }
    return enif_make_badarg(env);
}

static ERL_NIF_TERM echo(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[])
{
    UNUSED(env);
    UNUSED(argc);
    enif_fprintf(stderr, "echo argc=%d\r\n", argc);    
    return argv[0];
}

static ERL_NIF_TERM make_nil(ErlNifEnv* env)
{
    return enif_make_list(env, 0);
}

static ERL_NIF_TERM reverse(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[])
{
    ERL_NIF_TERM list = argv[0];
    ERL_NIF_TERM res = make_nil(env);
    ERL_NIF_TERM hd, tl;
    UNUSED(argc);

    enif_fprintf(stderr, "reverse argc=%d\r\n", argc);

    while(enif_get_list_cell(env, list, &hd, &tl)) {
	res = enif_make_list_cell(env, hd, res);
	list = tl;
    }
    if (enif_is_empty_list(env, list))
	return res;
    return enif_make_badarg(env);
}


ERL_NIF_INIT(demo, demo_funcs,
             0, 0, 0, 0)

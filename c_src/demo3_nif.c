//
// Demo3 resource types
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "erl_nif.h"

typedef struct demo3_state_t
{
    int value;
} demo3_state_t;

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

static ErlNifResourceType* point_res;
static ErlNifResourceType* rectangle_res;

typedef struct _point_t
{
    double x, y;
} Point;

typedef struct _dimension_t
{
    double w, h;
} Dimension;

typedef struct _rectangle_t
{
    Point     pos;
    Dimension dim;
} Rectangle;


static ERL_NIF_TERM add(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM get(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM new_point(ErlNifEnv* env,int argc,
			      const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM new_rectangle(ErlNifEnv* env,int argc,
				  const ERL_NIF_TERM argv[]);

ErlNifFunc demo3_funcs[] = {
    { "new_point",     2, new_point },
    { "new_rectangle", 4, new_rectangle },
    { "add",           2, add     },
    { "get",           1, get     },
};

static void point_dtor(ErlNifEnv* env, Point* p)
{
    UNUSED(env);
    enif_fprintf(stderr, "Point %p = (%f,%f) deallocated\r\n",
		 p, p->x, p->y);
}

static void rectangle_dtor(ErlNifEnv* env, Rectangle* r)
{
    UNUSED(env);
    enif_fprintf(stderr, "Rectangle %p = (%f,%f,%f,%f) deallocated\r\n",
		 r, r->pos.x, r->pos.y, r->dim.w, r->dim.h);
}

static int get_number(ErlNifEnv* env, ERL_NIF_TERM arg, double* dp)
{
    if (!enif_get_double(env, arg, dp)) {
	long lv;
	if (!enif_get_long(env, arg, &lv))
	    return 0;
	*dp = (double) lv;
    }
    return 1;
}

static ERL_NIF_TERM new_point(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[])
{
    Point* p;
    double x, y;
    ERL_NIF_TERM ret;
    
    if (!get_number(env, argv[0], &x))
	return enif_make_badarg(env);
    if (!get_number(env, argv[1], &y))
	return enif_make_badarg(env);
    if ((p = enif_alloc_resource(point_res, sizeof(Point))) == NULL)
	return enif_make_badarg(env); // memory exception?
    enif_fprintf(stderr, "sizeof point_res = %ld\r\n",
		 enif_sizeof_resource(p));
    p->x = x;
    p->y = y;
    ret = enif_make_resource(env, p);
    enif_release_resource(p);
    return ret;
}

static ERL_NIF_TERM new_rectangle(ErlNifEnv* env,int argc,
				  const ERL_NIF_TERM argv[])
{
    Rectangle* r;
    double x, y;
    double w, h;
    ERL_NIF_TERM ret;
    
    if (!get_number(env, argv[0], &x))
	return enif_make_badarg(env);
    if (!get_number(env, argv[1], &y))
	return enif_make_badarg(env);
    if (!get_number(env, argv[2], &w))
	return enif_make_badarg(env);
    if (!get_number(env, argv[3], &h))
	return enif_make_badarg(env);    
    if ((r = enif_alloc_resource(rectangle_res, sizeof(Rectangle))) == NULL)
	return enif_make_badarg(env); // memory exception?
    enif_fprintf(stderr, "sizeof rectangle_res = %ld\r\n",
		 enif_sizeof_resource(r));
    r->pos.x = x;
    r->pos.y = y;
    r->dim.w = w;
    r->dim.h = h;
    ret = enif_make_resource(env, r);
    enif_release_resource(r);
    return ret;
}

static ERL_NIF_TERM add(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[])
{
    Point* p1;
    Point* p2;
    Rectangle* r1;
    Rectangle* r2;
    
    if (enif_get_resource(env, argv[0], point_res, (void**) &p1)) {
	if (enif_get_resource(env, argv[1], point_res, (void**) &p2)) {
	    p2->x += p1->x;
	    p2->y += p1->y;
	}
	else if (enif_get_resource(env, argv[1], rectangle_res,(void**) &r2)) {
	    r2->pos.x += p1->x;
	    r2->pos.y += p1->y;
	}
	else
	    return enif_make_badarg(env);
    }
    if (enif_get_resource(env, argv[0], rectangle_res,(void**) &r1)) {
	if (enif_get_resource(env, argv[1], point_res,(void**) &p2)) {
	    p2->x += r1->pos.x;
	    p2->y += r1->pos.y;
	}
	else if (enif_get_resource(env, argv[1], rectangle_res,(void**) &r2)) {
	    r2->pos.x += r1->pos.x;
	    r2->pos.y += r2->pos.y;	    
	}
	else
	    return enif_make_badarg(env);	
    }
    return ATOM(ok);
}

static ERL_NIF_TERM get(ErlNifEnv* env,int argc,const ERL_NIF_TERM argv[])
{
    Point* p1;
    Rectangle* r1;
    if (enif_get_resource(env, argv[0], point_res, (void**) &p1)) {
	return enif_make_tuple2(env,
				enif_make_double(env, p1->x),
				enif_make_double(env, p1->y));
    }
    else if (enif_get_resource(env, argv[0], rectangle_res, (void**) &r1)) {
	return enif_make_tuple4(env,
				enif_make_double(env, r1->pos.x),
				enif_make_double(env, r1->pos.y),
				enif_make_double(env, r1->dim.w),
				enif_make_double(env, r1->dim.h));
    }
    return enif_make_badarg(env);
}

static int  demo3_load(ErlNifEnv* env, void** priv_data, ERL_NIF_TERM load_info)
{
    UNUSED(env);
    demo3_state_t* state;
    ErlNifResourceFlags tried;
    
    fprintf(stderr, "demo3_load called\n");

    if (!(state = enif_alloc(sizeof(demo3_state_t))))
	return -1;
    state->value = 1;

    // General atoms
    LOAD_ATOM(ok);
    LOAD_ATOM(error);
    LOAD_ATOM(unknown);
    LOAD_ATOM(undefined);
    LOAD_ATOM(true);
    LOAD_ATOM(false);

    point_res = enif_open_resource_type(env, 0, "point",
					(ErlNifResourceDtor*)point_dtor,
					ERL_NIF_RT_CREATE, &tried);

    rectangle_res = enif_open_resource_type(env, 0, "rectangle",
					    (ErlNifResourceDtor*)rectangle_dtor,
					    ERL_NIF_RT_CREATE, &tried);
    
    *priv_data = state;
    
    return 0;
}

static int demo3_upgrade(ErlNifEnv* env, void** priv_data, void** old_priv_data,
			 ERL_NIF_TERM load_info)
{
    UNUSED(env);
    fprintf(stderr, "demo3_upgrade called\n");
    *priv_data = *old_priv_data;
    return 0;
}

static void demo3_unload(ErlNifEnv* env, void* priv_data)
{
    UNUSED(env);
    demo3_state_t* state = priv_data;
    
    fprintf(stderr, "demo3_unload called\n");

    enif_free(state);
}


ERL_NIF_INIT(demo3, demo3_funcs,
	     demo3_load,
	     NULL,
	     demo3_upgrade,
	     demo3_unload)

#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/alloc.h"

#include "../data_structs.h"
#include "__builtin.h"

static decl *
builtin_unreachable()
{
	decl *d = decl_new();
	funcargs *fargs;

	d->type->primitive = type_void;

	d->desc = decl_desc_func_new(d, NULL);
	fargs = funcargs_new();
	fargs->args_void = 1; /* x(void) */

	d->desc->bits.func = fargs;

	d->spel = ustrdup("__builtin_unreachable");

	decl_attr_append(&d->attr, decl_attr_new(attr_noreturn));

	return d;
}

decl **builtin_funcs(void)
{
	decl **funcs = NULL, **i;

	dynarray_add((void ***)&funcs, builtin_unreachable());

	for(i = funcs; i && *i; i++)
		(*i)->builtin = 1;

	return funcs;
}

void builtin_fold(expr *e)
{
	/* funcall */
	(void)e;
}

void builtin_gen(expr *e)
{
	(void)e;
}
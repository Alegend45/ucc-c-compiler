#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "data_structs.h"
#include "typedef.h"

#include "tokenise.h"
#include "tokconv.h"

#include "sue.h"
#include "sym.h"

#include "cc1.h"

#include "parse.h"
#include "parse_type.h"

#include "expr.h"

type *parse_type_struct(void);
decl_desc *parse_decl_desc_array(enum decl_mode mode, char **sp);

#define INT_TYPE(t) do{ t = type_new(); t->primitive = type_int; }while(0)

void parse_type_preamble(type **tp, char **psp, enum type_primitive primitive)
{
	char *spel;
	type *t;

	spel = NULL;
	t = type_new();
	t->primitive = primitive;

	if(curtok == token_identifier){
		spel = token_current_spel();
		EAT(token_identifier);
	}

	*psp = spel;
	*tp = t;
}

/* sue = struct/union/enum */
type *parse_type_sue(enum type_primitive prim)
{
	type *t;
	char *spel;
	sue_member **members;

	parse_type_preamble(&t, &spel, prim);

	members = NULL;

	if(accept(token_open_block)){
		if(prim == type_enum){
			do{
				expr *e;
				char *sp;

				sp = token_current_spel();
				EAT(token_identifier);

				if(accept(token_assign))
					e = parse_expr_funcallarg(); /* no commas */
				else
					e = NULL;

				enum_vals_add(&members, sp, e);

				if(!accept(token_comma))
					break;
			}while(curtok == token_identifier);

			EAT(token_close_block);
		}else{
			members = (sue_member **)parse_decls_multi_type(DECL_CAN_DEFAULT | DECL_SPEL_NEED, 1);
			EAT(token_close_block);
		}

	}else if(!spel){
		die_at(NULL, "expected: struct definition or name");

	}else{
		/* predeclaring */
		if(prim == type_enum){
			if(!sue_find(current_scope, spel))
				cc1_warn_at(NULL, 0, WARN_PREDECL_ENUM, "predeclaration of enums is not c99");
		}
	}


	t->sue = sue_add(current_scope, spel, members, prim);

	return t;
}

type *parse_type()
{
	expr *tdef_typeof = NULL;
	enum type_qualifier qual = qual_none;
	enum type_storage   store = store_auto;
	enum type_primitive primitive = type_int;
	int is_signed = 1;
	int store_set = 0, primitive_set = 0, signed_set = 0;

	for(;;){
		decl *td;

		if(curtok_is_type_qual()){
			qual |= curtok_to_type_qualifier();
			EAT(curtok);

		}else if(curtok_is_type_store()){
			store = curtok_to_type_storage();

			if(store_set)
				die_at(NULL, "second type store %s", type_store_to_str(store));

			store_set = 1;
			EAT(curtok);

		}else if(curtok_is_type_primitive()){
			primitive = curtok_to_type_primitive();

			if(primitive_set)
				die_at(NULL, "second type primitive %s", type_primitive_to_str(primitive));

			primitive_set = 1;
			EAT(curtok);

		}else if(curtok == token_signed || curtok == token_unsigned){
			is_signed = curtok == token_signed;

			if(signed_set)
				die_at(NULL, "unwanted second \"%ssigned\"", is_signed ? "" : "un");

			signed_set = 1;
			EAT(curtok);

		}else if(curtok == token_struct || curtok == token_union || curtok == token_enum){
			const enum token tok = curtok;
			const char *str;
			type *t;

			EAT(curtok);

			switch(tok){
#define CASE(a)                           \
				case token_ ## a:                 \
					t = parse_type_sue(type_ ## a); \
					str = #a;                       \
					break

				CASE(enum);
				CASE(struct);
				CASE(union);

				default:
					ICE("wat");
			}

			if(signed_set || primitive_set)
				die_at(&t->where, "primitive/signed/unsigned with %s", str);

			t->qual  = qual;
			t->store = store;

			return t;

		}else if(accept(token_typeof)){
			if(primitive_set)
				die_at(NULL, "duplicate typeof specifier");

			tdef_typeof = parse_expr_sizeof_typeof();
			primitive_set = 1;

		}else if(curtok == token_identifier && (td = typedef_find(current_scope, token_current_spel_peek()))){
			/* typedef name */

			/*
			 * FIXME
			 * check for a following colon, in the case of
			 * typedef int x;
			 * x:;
			 *
			 * x is a valid label
			 */

			if(primitive_set){
				/* "int x" - we are at x, which is also a typedef somewhere */
				cc1_warn_at(NULL, 0, WARN_IDENT_TYPEDEF, "identifier is a typedef name");
				break;
			}

			/*if(tdef_typeof) - can't reach due to primitive_set */

			tdef_typeof = expr_new_sizeof_decl(td);
			primitive_set = 1;

			EAT(token_identifier);
		}else{
			break;
		}
	}

	if(qual != qual_none || store_set || primitive_set || signed_set || tdef_typeof){
		type *t = type_new();

		/* signed size_t x; */
		if(tdef_typeof && signed_set)
			die_at(NULL, "signed/unsigned not allowed with typedef instance (%s)", decl_spel(tdef_typeof->decl));


		if(!primitive_set)
			INT_TYPE(t); /* unsigned x; */
		else
			t->primitive = primitive;

		t->typeof = tdef_typeof;
		t->is_signed = is_signed;
		t->qual  = qual;
		t->store = store;

		return t;
	}else{
		return NULL;
	}
}

funcargs *parse_func_arglist()
{
	funcargs *args;
	decl *argdecl;

	args = funcargs_new();

	if(curtok == token_close_paren)
		goto empty_func;

	argdecl = parse_decl_single(0);

	if(argdecl){
		for(;;){
			dynarray_add((void ***)&args->arglist, argdecl);

			if(curtok == token_close_paren)
				break;

			EAT_OR_DIE(token_comma);

			if(accept(token_elipsis)){
				args->variadic = 1;
				break;
			}

			/* continue loop */
			/* actually, we don't need a type here, default to int, i think */
			argdecl = parse_decl_single(DECL_CAN_DEFAULT);
		}

		if(dynarray_count((void *)args->arglist) == 1
				&& args->arglist[0]->type->primitive == type_void
				&& !decl_ptr_depth(args->arglist[0])
				&& !decl_spel(args->arglist[0])){
			/* x(void); */
			function_empty_args(args);
			args->args_void = 1; /* (void) vs () */
		}

	}else{
		do{
			decl *d = decl_new();

			if(curtok != token_identifier)
				EAT(token_identifier); /* error */

			d->type->primitive = type_int;
			decl_set_spel(d, token_current_spel());
			dynarray_add((void ***)&args->arglist, d);

			EAT(token_identifier);

			if(curtok == token_close_paren)
				break;

			EAT_OR_DIE(token_comma);
		}while(1);
		args->args_old_proto = 1;
	}

empty_func:

	return args;
}

decl_desc *parse_decl_desc_ptr(enum decl_mode mode, char **sp)
{
	decl_desc *ret = NULL;

	while(accept(token_multiply)){
		enum type_qualifier qual = qual_none;
		decl_desc *ptr = decl_desc_ptr_new(NULL, NULL);

		while(curtok_is_type_qual()){
			qual |= curtok_to_type_qualifier();
			EAT(curtok);
		}

		ptr->bits.qual = qual;

		ptr->child = ret; /* check if we have anything else */

		ret = ptr;
	}

	if(accept(token_open_paren)){
		decl_desc *inside = parse_decl_desc_array(mode, sp);
		decl_desc *i;

		EAT(token_close_paren);

		/* tag on the bottom */
		if(ret){
			for(i = ret; i->child; i = i->child);
			i->child = inside;
		}else{
			ret = inside;
		}
	}

	if(curtok == token_identifier){
		if(mode & DECL_SPEL_NO)
			die_at(NULL, "identifier unexpected");

		*sp = token_current_spel();

		EAT(token_identifier);

	}else if(mode & DECL_SPEL_NEED){
		die_at(NULL, "need identifier for decl");
	}

	return ret;
}

decl_desc *parse_decl_desc_func(enum decl_mode mode, char **sp)
{
	decl_desc *ret = parse_decl_desc_ptr(mode, sp);

	while(accept(token_open_paren)){
		/*
		 * e.g.:
		 * int (*x)(
		 *         ^
		 */
		decl_desc *dp = decl_desc_func_new(NULL, NULL);

		dp->bits.func = parse_func_arglist();

		EAT(token_close_paren);

		if(dp)
			dp->child = ret;

		ret = dp;
	}

	return ret;
}

decl_desc *parse_decl_desc_array(enum decl_mode mode, char **sp)
{
	decl_desc *dp = parse_decl_desc_func(mode, sp);

	while(accept(token_open_square)){
		decl_desc *dp_new;
		expr *size;

		if(accept(token_close_square)){
			/* take size as zero */
			size = expr_new_val(0);
			/* TODO: break */
		}else{
			/* fold.c checks for const-ness */
			size = parse_expr();
			EAT(token_close_square);
		}

		dp_new = decl_desc_array_new(NULL, NULL);

		dp_new->bits.array_size = size;

		if(dp)
			dp->child = dp_new;

		dp = dp_new;
	}

	return dp;
}

#include "parse_attr.c"

decl *parse_decl(type *t, enum decl_mode mode)
{
	decl_desc *dp;
	decl *d;
	char *spel = NULL;

	dp = parse_decl_desc_array(mode, &spel);

	/* don't fold typedefs until later (for __typeof) */
	d = decl_new();
	d->desc = dp;
	d->type = type_copy(t);
	d->spel = spel;

	decl_desc_link(d);

	if(accept(token_attribute)){
		EAT(token_open_paren);
		EAT(token_open_paren);

		if(curtok != token_close_paren)
			d->attr = parse_attr();

		EAT(token_close_paren);
		EAT(token_close_paren);
	}

	if(decl_spel(d) && accept(token_assign)){
		d->init = parse_expr_funcallarg(); /* int x = 5, j; - don't grab the comma expr */
	}else if(decl_is_func(d) && curtok != token_semicolon){
		/* optionally check for old func decl */
		decl **old_args = PARSE_DECLS();

		if(old_args){
			/* check then replace old args */
			int n_proto_decls, n_old_args;
			int i;
			funcargs *dfuncargs = d->desc->bits.func;

			if(!dfuncargs->args_old_proto)
				die_at(&d->where, "unexpected old-style decls - new style proto used");

			n_proto_decls = dynarray_count((void **)dfuncargs->arglist);
			n_old_args = dynarray_count((void **)old_args);

			if(n_old_args > n_proto_decls)
				die_at(&d->where, "old-style function decl: too many decls");

			for(i = 0; i < n_old_args; i++)
				if(old_args[i]->init)
					die_at(&old_args[i]->where, "parameter \"%s\" is initialised", decl_spel(old_args[i]));

			for(i = 0; i < n_old_args; i++){
				int j;
				for(j = 0; j < n_proto_decls; j++){
					if(!strcmp(decl_spel(old_args[i]), decl_spel(dfuncargs->arglist[j]))){
						decl **replace_this;
						decl *free_this;

						/* replace the old implicit int arg */
						replace_this = &dfuncargs->arglist[j];

						free_this = *replace_this;
						*replace_this = old_args[i];

						decl_free(free_this);
						break;
					}
				}
			}

			free(old_args);

			if(curtok != token_open_block)
				die_at(&dfuncargs->where, "no code for old-style function");
		}

		d->func_code = parse_code();
	}

	return d;
}

decl *parse_decl_single(enum decl_mode mode)
{
	type *t = parse_type();

	if(!t){
		if(mode & DECL_CAN_DEFAULT){
			INT_TYPE(t);
			cc1_warn_at(&t->where, 0, WARN_IMPLICIT_INT, "defaulting type to int");
		}else{
			return NULL;
		}
	}

	if(t->store == store_typedef)
		die_at(&t->where, "typedef unexpected");

	return parse_decl(t, mode);
}

decl **parse_decls_one_type()
{
	type *t = parse_type();
	decl **decls = NULL;

	if(!t)
		return NULL;

	if(t->store == store_typedef)
		die_at(&t->where, "typedef unexpected");

	do{
		decl *d = parse_decl(t, DECL_SPEL_NEED);
		dynarray_add((void ***)&decls, d);
	}while(accept(token_comma));

	return decls;
}

decl **parse_decls_multi_type(const int can_default, const int accept_field_width)
{
	const enum decl_mode parse_flag = can_default ? DECL_CAN_DEFAULT : 0;
	decl **decls = NULL;
	decl *last;
	int are_tdefs;

	/* read a type, then *spels separated by commas, then a semi colon, then repeat */
	for(;;){
		type *t;

		last = NULL;
		are_tdefs = 0;

		t = parse_type();

		if(!t){
			/* can_default makes sure we don't parse { int *p; *p = 5; } the latter as a decl */
			if(parse_possible_decl() && can_default){
				INT_TYPE(t);
				cc1_warn_at(&t->where, 0, WARN_IMPLICIT_INT, "defaulting type to int");
			}else{
				return decls;
			}
		}

		if(t->store == store_typedef)
			are_tdefs = 1;
		else if(t->sue && !parse_possible_decl())
			goto next; /*
									* struct { int i; }; - continue to next one
									* we check the actual ->struct/enum because we don't allow "enum x;"
									*/

		do{
			decl *d = parse_decl(t, parse_flag);

			UCC_ASSERT(d, "null decl after parse");

			if(!decl_spel(d)){
				/*
				 * int; - fine for "int;", but "int i,;" needs to fail
				 * struct A; - fine
				 */
				if(!last){
					int warn = 0;

					switch(t->primitive){
						case type_struct:
						case type_union:
						case type_enum:
							warn = t->sue && !t->sue->anon;
							break;
						default:
							warn = 1;
					}

					if(warn)
						warn_at(&d->where, "declaration doesn't declare anything");

					decl_free_notype(d);
					goto next;
				}
				die_at(&d->where, "identifier expected after decl");
			}

			dynarray_add(are_tdefs
					? (void ***)&current_scope->typedefs
					:  (void ***)&decls,
					d);

			if(are_tdefs){
				if(decl_is_func(d) && d->func_code)
					die_at(&d->where, "can't have a typedef function with code");
				else if(d->init)
					die_at(&d->where, "can't init a typedef");
			}

			if(accept_field_width && accept(token_colon)){
				/* normal decl, check field spec */
				d->field_width = currentval.val;
				if(d->field_width <= 0)
					die_at(&d->where, "field width must be positive");
				EAT(token_integer);
			}

			last = d;
		}while(accept(token_comma));

		if(last && !last->func_code){
next:
			/* end of type, if we have an identifier, '(' or '*', it's an unknown type name */
			switch(curtok){
				case token_identifier:
				case token_open_paren:
				case token_multiply:
					if(last)
						die_at(NULL, "unknown type name '%s'", decl_spel(last));
					/* else die below */
				default:
					break;
			}
			EAT(token_semicolon);
		}
	}
}

#include "ops.h"
#include "expr_stmt.h"

const char *str_expr_block(void)
{
	return "block";
}

static void got_stmt(stmt *current, int *stop, void *extra)
{
	if(stmt_kind(current, return)){
		stmt **store = extra;
		*store = current;
		*stop = 1;
	}
}

void fold_expr_block(expr *e, symtable *stab)
{
	decl_desc *func;
	stmt *r;

	/* add e->block_args to symtable */

	symtab_add_args(e->code->symtab, e->block_args, "block-function");

	fold_stmt(e->code); /* symtab set by parse */

	UCC_ASSERT(stmt_kind(e->code, code), "!code for block");

	/*
	 * search for a return
	 * if none: void
	 * else the type of the first one we find
	 */

	e->tree_type = decl_new();
	e->tree_type->type->store = store_static;

	r = NULL;
	stmt_walk(e->code, got_stmt, &r);
	if(r && r->expr)
		decl_copy_primitive(e->tree_type, r->expr->tree_type);
	else
		e->tree_type->type->primitive = type_void;

	/* copied the type, now make it a function */
	func = decl_desc_func_new(NULL, NULL);

	decl_desc_append(&e->tree_type->desc, func);
	decl_desc_append(&e->tree_type->desc, decl_desc_block_new(NULL, NULL));
	decl_desc_link(e->tree_type);

	func->bits.func = e->block_args;

	/* add the function to the global scope */
	e->tree_type->spel = asm_label_block(curdecl_func->spel);
	e->sym = SYMTAB_ADD(symtab_root(stab), e->tree_type, sym_global);

	e->tree_type->func_code = e->code;
}

void gen_expr_block(expr *e, symtable *stab)
{
	(void)stab;

	/* load the function pointer */
	asm_temp(1, "mov rax, %s", e->sym->decl->spel);
	asm_temp(1, "push rax");
}

void gen_expr_str_block(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("block, type: %s, code:\n", decl_to_str(e->tree_type));
	gen_str_indent++;
	print_stmt(e->code);
	gen_str_indent--;
}

void gen_expr_style_block(expr *e, symtable *stab)
{
	(void)e;
	(void)stab;
	/* TODO */
}

void mutate_expr_block(expr *e)
{
	(void)e;
}

expr *expr_new_block(funcargs *args, stmt *code)
{
	expr *e = expr_new_wrapper(block);
	e->block_args = args;
	e->code = code;
	return e;
}
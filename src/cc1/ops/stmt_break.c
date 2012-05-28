#include "ops.h"
#include "stmt_break.h"

const char *str_stmt_break()
{
	return "break";
}

void fold_stmt_break_continue(stmt *t, char *lbl)
{
	if(!lbl)
		die_at(&t->where, "%s outside a flow-control statement", t->f_str());

	t->expr = expr_new_identifier(lbl);
	t->expr->tree_type = decl_new();
	t->expr->tree_type->type->primitive = type_void;
}

void fold_stmt_break(stmt *t)
{
	fold_stmt_break_continue(t, t->parent ? t->parent->lbl_break : NULL);
}

func_gen_stmt *gen_stmt_break = gen_stmt_goto;

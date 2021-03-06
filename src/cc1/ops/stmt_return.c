#include "ops.h"
#include "stmt_return.h"

const char *str_stmt_return()
{
	return "return";
}

void fold_stmt_return(stmt *s)
{
	const int void_func = decl_is_void(curdecl_func_called);

	if(s->expr){
		fold_expr(s->expr, s->symtab);
		fold_need_expr(s->expr, "return", 0);

		fold_decl_equal(s->expr->tree_type, curdecl_func_called,
				&s->where, WARN_RETURN_TYPE,
				"mismatching return type for %s (%s)",
				curdecl_func->spel,
				decl_to_str(s->expr->tree_type));

		if(void_func)
			cc1_warn_at(&s->where, 0, 1, WARN_RETURN_TYPE,
					"return with a value in void function %s", curdecl_func->spel);

	}else if(!void_func){
		cc1_warn_at(&s->where, 0, 1, WARN_RETURN_TYPE,
				"empty return in non-void function %s", curdecl_func->spel);

	}
}

void gen_stmt_return(stmt *s)
{
	if(s->expr){
		gen_expr(s->expr, s->symtab);
		asm_temp(1, "pop rax ; return");
	}
	asm_temp(1, "jmp %s", curfunc_lblfin);
}

void mutate_stmt_return(stmt *s)
{
	s->f_passable = fold_passable_no;
}

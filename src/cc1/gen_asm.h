#ifndef GEN_ASM_H
#define GEN_ASM_H

extern char *curfunc_lblfin;

void gen_asm_global(decl *d);

void gen_asm(symtable *);
void gen_expr(expr *e, symtable *stab); // FIXME: remove stab param
void gen_stmt(stmt *t);

#endif

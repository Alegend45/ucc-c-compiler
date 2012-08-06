#ifndef OUT_H
#define OUT_H

void out_pop(void);
void out_pop_func_ret(decl *d);

void out_push_iv(decl *d, intval *iv);
void out_push_i( decl *d, int i);
void out_push_lbl(char *s, int pic); /* implicitly pointer */

void out_dup(void); /* duplicate top of stack */
void out_normalise(void); /* change to 0 or 1 */

void out_push_sym_addr(sym *);
void out_push_sym(sym *);
void out_store(void); /* store stack[1] into *stack[0] */

void out_op(      enum op_type, decl *d); /* binary ops and comparisons */
void out_op_unary(enum op_type, decl *d); /* unary ops */

void out_cast(decl *from, decl *to);

void out_call_start(void);
void out_call(void); /* call *pop(), push result */
void out_call_fin(int); /* remove args from stack */

void out_jmp( void); /* jmp to *pop() */
void out_jz( const char *);
void out_jnz(const char *);

void out_func_prologue(int offset); /* push rbp, sub rsp, ... */
void out_func_epilogue(void); /* mov rsp, rbp; ret */
void out_label(const char *);

void out_comment(const char *, ...);

#endif

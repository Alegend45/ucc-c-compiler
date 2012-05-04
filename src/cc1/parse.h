#ifndef PARSE_H
#define PARSE_H

enum decl_mode
{
	DECL_SPEL_NEED    = 1 << 0,
	DECL_SPEL_NO      = 1 << 1,
	DECL_CAN_DEFAULT  = 1 << 2,
};

enum decl_multi_mode
{
	DECL_MULTI_CAN_DEFAULT        = 1 << 0,
	DECL_MULTI_ACCEPT_FIELD_WIDTH = 1 << 1,
	DECL_MULTI_ACCEPT_FUNCTIONS   = 1 << 2,
};

extern enum token curtok;

#define PARSE_DECLS() parse_decls_multi_type(0)

#define parse_expr() parse_expr_comma()
#define parse_expr_funcallarg() parse_expr_if()
#define parse_possible_decl() (curtok == token_identifier || curtok == token_multiply || curtok == token_open_paren)
expr *parse_expr();

stmt  *parse_code(void);

expr **parse_funcargs(void);
expr *parse_expr_binary_op(void); /* needed to limit [+-] parsing */
expr *parse_expr_array(void);
expr *parse_expr_if(void);
expr *parse_expr_deref(void);
expr *parse_expr_sizeof_typeof(void);

symtable *parse(void);

/* tables local to the current scope */
extern symtable *current_scope;

#endif

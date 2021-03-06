#ifndef PARSE_TYPE_H
#define PARSE_TYPE_H

type *parse_type(int with_store);

/* (type) (ident) */
decl  *parse_decl_single(enum decl_mode mode);

/* type ident(, ident, ...) */
decl **parse_decls_one_type(void);

/* type ident...; type ident...; */
decl **parse_decls_multi_type(enum decl_multi_mode mode);

funcargs *parse_func_arglist(void);

#endif

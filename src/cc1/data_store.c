#include <stdarg.h>
#include <stdio.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "data_structs.h"
#include "data_store.h"
#include "asm.h"
#include "str.h"

data_store *data_store_new_str(char *s, int l)
{
	data_store *ds = umalloc(sizeof *ds);

	ds->type     = data_store_str;
	ds->bits.str = s;
	ds->len      = l;

	return ds;
}

void data_store_declare(data_store *ds, FILE *f)
{
	fprintf(f, "%s:\ndb ", ds->spel);
}

void data_store_out(data_store *ds, FILE *f)
{
	int i;

	switch(ds->type){
		case data_store_str:
			for(i = 0; i < ds->len; i++)
				fprintf(f, "%d%s", ds->bits.str[i], i == ds->len - 1 ? "" : ", ");
			fputc('\n', f);
			break;
	}
}

void data_store_fold_decl(data_store *ds, decl **ptree_type)
{
	decl *tree_type = decl_new();
	char *sym_spel;

	tree_type->desc = decl_desc_array_new(tree_type, NULL);
	tree_type->desc->bits.array_size = expr_new_val(ds->len);

	tree_type->type->store = store_static;
	/*tree_type->type->qual  = qual_const; - no thanks*/

	switch(ds->type){
		case data_store_str:
			sym_spel = "str";
			tree_type->type->primitive = type_char;
			break;
	}

	ds->spel = asm_label_data_store(sym_spel);

	*ptree_type = tree_type;
}
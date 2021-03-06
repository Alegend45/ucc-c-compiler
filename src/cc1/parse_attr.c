decl_attr *parse_attr_format()
{
	/* __attribute__((format (printf, fmtarg, firstvararg))) */
	decl_attr *da;
	char *func;

	EAT(token_open_paren);

	func = token_current_spel();
	EAT(token_identifier);

	da = decl_attr_new(attr_format);

#define CHECK(s) !strcmp(func, s) || !strcmp(func, "__" s "__")

	if(CHECK("printf"))
		da->attr_extra.format.fmt_func = attr_fmt_printf;
	else if(CHECK("scanf"))
		da->attr_extra.format.fmt_func = attr_fmt_scanf;
	else
		DIE_AT(&da->where, "unknown format func \"%s\"", func);

	EAT(token_comma);

	da->attr_extra.format.fmt_arg = currentval.val;
	EAT(token_integer);

	EAT(token_comma);

	da->attr_extra.format.var_arg = currentval.val;
	EAT(token_integer);

	EAT(token_close_paren);

	return da;
}

decl_attr *parse_attr_section()
{
	/* __attribute__((section ("sectionname"))) */
	decl_attr *da;
	char *func;
	int len, i;

	EAT(token_open_paren);

	if(curtok != token_string)
		DIE_AT(NULL, "string expected for section");

	token_get_current_str(&func, &len);
	EAT(token_string);

	for(i = 0; i < len; i++)
		if(!isprint(func[i])){
			if(i < len - 1 || func[i] != '\0')
				warn_at(NULL, 1, "character 0x%x detected in section", func[i]);
			break;
		}

	da = decl_attr_new(attr_section);

	da->attr_extra.section = func;

	EAT(token_close_paren);

	return da;
}

#define EMPTY(t)                      \
decl_attr *parse_ ## t()              \
{                                     \
	return decl_attr_new(t);            \
}

EMPTY(attr_unused)
EMPTY(attr_warn_unused)
EMPTY(attr_enum_bitmask)
EMPTY(attr_noreturn)
EMPTY(attr_noderef)

static struct
{
	const char *ident;
	decl_attr *(*parser)(void);
} attrs[] = {
	{ "format",         parse_attr_format },
	{ "unused",         parse_attr_unused },
	{ "warn_unused",    parse_attr_warn_unused },
	{ "section",        parse_attr_section },
	{ "bitmask",        parse_attr_enum_bitmask },
	{ "noreturn",       parse_attr_noreturn },
	{ "noderef",        parse_attr_noderef },
	{ NULL, NULL },
};
#define MAX_FMT_LEN 16

void parse_attr_bracket_chomp(void)
{
	if(accept(token_open_paren)){
		parse_attr_bracket_chomp(); /* nest */

		while(curtok != token_close_paren)
			EAT(curtok);

		EAT(token_close_paren);
	}
}

decl_attr *parse_attr_single(char *ident)
{
	int i;

	for(i = 0; attrs[i].ident; i++){
		char buf[MAX_FMT_LEN];
		if(!strcmp(attrs[i].ident, ident)
		|| (snprintf(buf, sizeof buf, "__%s__", attrs[i].ident), !strcmp(buf, ident)))
		{
			return attrs[i].parser();
		}
	}

	warn_at(NULL, 1, "ignoring unrecognised attribute \"%s\"", ident);

	/* if there are brackets, eat them all */

	parse_attr_bracket_chomp();

	return NULL;
}

decl_attr *parse_attr(void)
{
	decl_attr *attr = NULL, **next = &attr;

	for(;;){
		char *ident;

		if(curtok != token_identifier)
			DIE_AT(NULL, "identifier expected for attribute (got %s)", token_to_str(curtok));

		ident = token_current_spel();
		EAT(token_identifier);

		if((*next = parse_attr_single(ident)))
			next = &(*next)->next;

		free(ident);

		if(!accept(token_comma))
			break;
	}

	return attr;
}

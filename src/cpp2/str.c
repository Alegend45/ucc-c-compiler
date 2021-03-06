#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "str.h"
#include "../util/alloc.h"
#include "../util/util.h"
#include "macro.h"

static int iswordpart(char c)
{
	return isalnum(c) || c == '_';
}

void str_trim(char *str)
{
	char *s;

	for(s = str; isspace(*s); s++);

	memmove(str, s, strlen(s) + 1);

	for(s = str; *s; s++);

	while(s > str && isspace(*s))
		s--;

	*s = '\0';
}

char *str_join(char **strs, const char *with)
{
	const int with_len = strlen(with);
	int len;
	char **itr;
	char *ret, *p;

	len = 1; /* \0 */
	for(itr = strs; *itr; itr++)
		len += strlen(*itr) + with_len;

	p = ret = umalloc(len);

	for(itr = strs; *itr; itr++)
		p += sprintf(p, "%s%s", *itr, itr[1] ? with : "");

	return ret;
}

char *word_dup(const char *s)
{
	const char *start = s;
	while(iswordpart(*s))
		s++;
	return ustrdup2(start, s);
}

char *str_quote(const char *quoteme)
{
	int len;
	const char *s;
	char *ret, *p;

	len = 3; /* ""\0 */
	for(s = quoteme; *s; s++, len++)
		if(*s == '"')
			len++;

	p = ret = umalloc(len);

	*p++ = '"';

	for(s = quoteme; *s; s++){
		if(*s == '"')
			*p++ = '\\';
		*p++ = *s;
	}

	strcpy(p, "\"");

	return ret;
}

char *str_replace(char *line, char *start, char *end, const char *replace)
{
	const unsigned int len_find    = end - start;
	const unsigned int len_replace = strlen(replace);

	if(len_replace < len_find){
		/* start<->end distance is less than replace.length */
		memcpy(start, replace, len_replace);
		memmove(start + len_replace, end, strlen(end) + 1);
		return line;
	}else{
		char *del = line;

		*start = '\0';
		line = ustrprintf("%s%s%s", line, replace, end);
		free(del);

		return line;
	}
}

char *word_replace(char *line, char *pos, const char *find, const char *replace)
{
	return str_replace(line, pos, pos + strlen(find), replace);
}

int word_replace_g(char **pline, char *find, const char *replace)
{
	char *pos = *pline;
	int r = 0;

	DEBUG(DEBUG_VERB, "word_find(\"%s\", \"%s\")\n", pos, find);

	while((pos = word_find(pos, find))){
		int posidx = pos - *pline;

		DEBUG(DEBUG_VERB, "word_replace(line=\"%s\", pos=\"%s\", nam=\"%s\", val=\"%s\")\n",
				*pline, pos, find, replace);

		*pline = word_replace(*pline, pos, find, replace);
		pos = *pline + posidx + strlen(replace);

		r = 1;
	}

	return r;
}

char *word_strstr(char *haystack, char *needle)
{
	const int nlen = strlen(needle);
	char *i;

	if(!strstr(haystack, needle))
		return NULL;

	for(i = haystack; *i; i++)
		if(*i == '"'){
refind:
			i = strchr(i + 1, '"');
			if(!i)
				ICE("terminating quote not found\nhaystack = >>%s<<\nneedle = >>%s<<",
						haystack, needle);
			else if(i[-1] == '\\')
				goto refind;
		}else if(!strncmp(i, needle, nlen)){
			return i;
		}

	return NULL;
}

char *word_find(char *line, char *word)
{
	const int wordlen = strlen(word);
	char *pos = line;

	while((pos = word_strstr(pos, word))){
		char *fin;
		if(pos > line && iswordpart(pos[-1])){
			pos++;
			continue;
		}
		fin = pos + wordlen;
		if(iswordpart(*fin)){
			pos++;
			continue;
		}
		return pos;
	}
	return NULL;
}

char *nest_close_paren(char *start)
{
	int nest = 0;

	while(*start){
		switch(*start){
			case '(':
				nest++;
				break;
			case ')':
				if(nest-- == 0)
					return start;
		}
		start++;
	}

	return NULL;
}

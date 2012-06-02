#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>

#include "ucc.h"
#include "ucc_ext.h"
#include "ucc_lib.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"

struct
{
	int assume_c;
	int nostdlib;
	int nostartfiles;
} gopts;

enum mode
{
	mode_preproc,
	mode_compile,
	mode_assemb,
	mode_link
};
#define MODE_ARG_CH(m) ("ESc\0"[m])

struct cc_file
{
	char *in;

	char *preproc,
			 *compile,
			 *assemb;

	char *out;
};
#define FILE_IN_MODE(f)        \
((f)->preproc ? mode_preproc : \
 (f)->compile ? mode_compile : \
 (f)->assemb  ? mode_assemb  : \
 mode_link)

const char *argv0;

char *tmpfilenam()
{
	char *r = tmpnam(NULL);

	if(!r)
		die("tmpfile():");

	return ustrdup(r);
}

void create_file(struct cc_file *file, enum mode mode, char *in)
{
	char *ext;

	file->in = in;

	if(gopts.assume_c)
		goto preproc;

#define ASSIGN(x)                \
				file->x = tmpfilenam();  \
				if(mode == mode_ ## x){  \
					file->out = file->x;   \
					return;                \
				}

	ext = strrchr(in, '.');
	if(ext && ext[1] && !ext[2]){
		switch(ext[1]){
preproc:
			case 'c':
				ASSIGN(preproc);
			case 'i':
				ASSIGN(compile);
			case 's':
				ASSIGN(assemb);
				file->out = file->assemb;
				break;

			default:
			assume_obj:
				fprintf(stderr, "assuming \"%s\" is object-file\n", in);
			case 'o':
				/* else assume it's already an object file */
				file->out = file->in;
		}
	}else{
		goto assume_obj;
	}
}

void free_file(struct cc_file *file)
{
#define DEL_FREE(fn) remove(fn); free(fn)
	/*free(file->in);*/
	DEL_FREE(file->preproc);
	DEL_FREE(file->compile);
	DEL_FREE(file->assemb);
	/*free(file->out);*/
}

void gen_obj_file(struct cc_file *file, char **args[4], enum mode mode)
{
	char *in = file->in;

	if(file->preproc){
		preproc(in, file->preproc, args[mode_preproc]);

		if(mode == mode_preproc)
			return;

		in = file->preproc;
	}

	if(file->compile){
		compile(in, file->compile, args[mode_compile]);

		if(mode == mode_compile)
			return;

		in = file->compile;
	}

	if(file->assemb){
		assemble(in, file->assemb, args[mode_assemb]);
	}
}

void rename_files(struct cc_file *files, int nfiles, char *output, enum mode mode)
{
	const char mode_ch = MODE_ARG_CH(mode);
	int i;

	for(i = 0; i < nfiles; i++){
		/* path/to/input.c -> path/to/input.[os] */
		char *new;

		if(mode < mode_link && FILE_IN_MODE(&files[i]) == mode_link){
			fprintf(stderr, "linker input \"%s\" unused with -%c present\n",
					files[i].in, mode_ch);
			continue;
		}

		if(output){
			if(mode == mode_preproc){
				/* append to current */
				if(files[i].preproc)
					cat(files[i].preproc, output, i);
				continue;
			}else if(mode == mode_compile && !strcmp(output, "-")){
				cat(files[i].compile, NULL, 0);
				continue;
			}

			new = ustrdup(output);
		}else{
			switch(mode){
				case mode_link:
					die("mode_link for multiple files");

				case mode_preproc:
					if(files[i].preproc)
						cat(files[i].preproc, NULL, 0);
					continue;

				case mode_compile:
				case mode_assemb:
				{
					int len;
					new = ustrdup(files[i].in);
					len = strlen(new);
					new[len - 1] = mode == mode_compile ? 's' : 'o';
					break;
				}
			}
		}

		rename_or_move(files[i].out, new);

		free(new);
	}
}

void process_files(enum mode mode, char **inputs, char *output, char **args[4], char *backend)
{
	const int ninputs = dynarray_count((void **)inputs);
	int i;
	struct cc_file *files;
	char **links;

	if(backend)
		die("TODO: backend");

	files = umalloc(ninputs * sizeof *files);

	links = gopts.nostdlib ? NULL : objfiles_stdlib();

	if(!gopts.nostartfiles)
		dynarray_add((void ***)&links, objfiles_start());


	for(i = 0; i < ninputs; i++){
		create_file(&files[i], mode, inputs[i]);

		gen_obj_file(&files[i], args, mode);

		dynarray_add((void ***)&links, files[i].out);
	}

	if(mode == mode_link)
		link_all(links, output ? output : "a.out", args[mode_link]);
	else
		rename_files(files, ninputs, output, mode);

	dynarray_free((void ***)&links, NULL);
	/* technically we need to free the ones from objfiles_... */

	for(i = 0; i < ninputs; i++)
		free_file(&files[i]);

	free(files);
}

void die(const char *fmt, ...)
{
	const int len = strlen(fmt);
	const int err = len > 0 && fmt[len - 1] == ':';
	va_list l;

	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);

	if(err)
		fprintf(stderr, " %s", strerror(errno));

	fputc('\n', stderr);

	exit(1);
}

void ice(const char *fmt)
{
	die("ICE: %s", fmt);
}

int main(int argc, char **argv)
{
	enum mode mode = mode_link;
	int i;
	char **inputs = NULL;
	char **args[4] = { 0 };
	char *output = NULL;
	char *backend = NULL;
	struct
	{
		char optn;
		char **ptr;
	} opts[] = {
		{ 'o', &output   },
		{ 'X', &backend  },
	};

	argv0 = argv[0];

	if(argc <= 1){
		fprintf(stderr, "Usage: %s [options] input(s)\n", *argv);
		return 1;
	}

	for(i = 1; i < argc; i++){
		if(!strcmp(argv[i], "--")){
			while(++i < argc)
				dynarray_add((void ***)&inputs, argv[i]);
			break;

		}else if(*argv[i] == '-'){
			int found = 0;
			unsigned int j;
			char *arg = argv[i];

			switch(arg[1]){
				case 'W':
				{
					/* check for W%c, */
					char c;
					if(sscanf(arg, "-W%c,", &c) == 1){
						switch(c){
#define MAP(c, l) case c: arg += 4; goto arg_ ## l;
							MAP('p', cpp);
							MAP('a', asm);
							MAP('l', ld);
#undef MAP
							default:
								fprintf(stderr, "argument \"%s\" assumed to be for cc1\n", arg);
						}
					}
					/* else default to -Wsomething - add to cc1 */
				}

#define ADD_ARG(to) dynarray_add((void ***)&args[to], ustrdup(arg))

				case 'w':
				case 'f':
/*arg_cc1:*/
					ADD_ARG(mode_compile);
					continue;

				case 'D':
				case 'U':
				case 'I':
arg_cpp:
					ADD_ARG(mode_preproc);
					continue;

arg_asm:
					ADD_ARG(mode_assemb);
					continue;

				case 'l':
				case 'L':
arg_ld:
					ADD_ARG(mode_link);
					continue;

#define CHECK_1() if(argv[i][2]) goto unrec;
				case 'E': CHECK_1(); mode = mode_preproc; continue;
				case 'S': CHECK_1(); mode = mode_compile; continue;
				case 'c': CHECK_1(); mode = mode_assemb;  continue;

				case 'o':
					if(argv[i][2]){
						output = argv[i] + 2;
					}else{
						output = argv[++i];
						if(!output)
							die("output argument needed");
					}
					continue;

				case 'x':
					/* prevent implicit assumption of source */
					if(!argv[++i] || strcmp(argv[i], "c"))
						die("-x only accepts \"c\"");
					gopts.assume_c = 1;
					continue;

				default:
					if(!strcmp(argv[i], "-nostdlib"))
						gopts.nostdlib = 1;
					else if(!strcmp(argv[i], "-nostartfiles"))
						gopts.nostartfiles = 1;
					else if(!strcmp(argv[i], "-###"))
						ucc_ext_cmds_show(1), ucc_ext_cmds_noop(1);
					else if(!strcmp(argv[i], "-v"))
						ucc_ext_cmds_show(1);
					else
						break;

					continue;
			}

			for(j = 0; j < sizeof(opts) / sizeof(opts[0]); j++)
				if(argv[i][1] == opts[j].optn){
					if(argv[i][2]){
						*opts[j].ptr = argv[i] + 2;
					}else{
						if(!argv[++i])
							die("need argument for %s", argv[i - 1]);
						*opts[j].ptr = argv[i];
					}
					found = 1;
					break;
				}

			if(!found)
unrec:	 die("unrecognised option \"%s\"", argv[i]);
		}else{
			dynarray_add((void ***)&inputs, argv[i]);
		}
	}

	if(output && dynarray_count((void **)inputs) > 1 && (mode == mode_compile || mode == mode_assemb))
		die("can't specify '-o' with '-%c' and an output", MODE_ARG_CH(mode));


	if(output && mode == mode_preproc && !strcmp(output, "-"))
		output = NULL;
	/* other case is -S, which is handled elsewhere */


	/* got arguments, a mode, and files to link */
	process_files(mode, inputs, output, args, backend);

	for(i = 0; i < 4; i++)
		dynarray_free((void ***)&args[i], free);
	dynarray_free((void ***)&inputs, NULL);

	return 0;
}

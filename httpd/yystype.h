#ifndef H_YYSTYPE
#define H_YYSTYPE

typedef struct {
	union {
		int  n;
		char *s;
	} v;
} YYSTYPE;

struct {
	char const *name;
	int error;
	size_t lineno;
} file;

YYSTYPE yylval;
int yylex(void);

/* bison need this */
#define YYSTYPE_IS_DECLARED 1

/*
 * disable flex warning
 * <stdout>: warning: ‘input’ defined but not used
*/
#define YY_NO_INPUT 1


#endif /* H_YYSTYPE */

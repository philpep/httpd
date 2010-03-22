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


#endif /* H_YYSTYPE */

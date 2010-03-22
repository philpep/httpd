#ifndef H_TOOLS
#define H_TOOLS

char **splitstr(char *, const char *, size_t *);
char *unsplit(char **, char *);
int zasprintf(char **, const char *, ...);
void zwrite(int, const char *, ...);
char *getdate(char *);


#endif /* H_TOOLS */

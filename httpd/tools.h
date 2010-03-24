#ifndef H_TOOLS
#define H_TOOLS

char **splitstr(char *, const char *, size_t *);
char *unsplit(char **, char *);
int zasprintf(char **, const char *, ...);
void zwrite(int, const char *, ...);
char *get_date(char *);
const char *get_mime_type(char *);


#endif /* H_TOOLS */

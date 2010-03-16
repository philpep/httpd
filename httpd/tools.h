#ifndef H_TOOLS
#define H_TOOLS

char **splitstr(char *str, const char *sep, size_t *n);
int zasprintf(char **ptr, const char *fmt, ...);
void zwrite(int fd, const char *fmt, ...);
char *getdate(char *date);


#endif /* H_TOOLS */

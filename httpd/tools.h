#ifndef H_TOOLS
#define H_TOOLS

char **splitstr(struct Client *, char *, const char *, size_t *);
int zasprintf(struct Client *, char **, const char *, ...);
void zwrite(struct Client *, const char *, ...);
char *get_date(char *);
const char *get_mime_type(char *);
const char *get_ipstring(struct sockaddr_storage *, char *);


#endif /* H_TOOLS */

#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
static size_t cw(char const*s,char c){size_t n=0;while(*s){while(*s==c)s++;if(*s)n++;while(*s&&*s!=c)s++;}return n;}
char **ft_split(char const *s,char c){size_t n=cw(s,c),i=0,l;char**t=malloc((n+1)*sizeof(char*));if(!t)return 0;while(i<n){while(*s==c)s++;l=0;while(s[l]&&s[l]!=c)l++;t[i]=ft_substr(s,0,l);if(!t[i]){while(i--)free(t[i]);free(t);return 0;}s+=l;i++;}t[n]=0;return t;}

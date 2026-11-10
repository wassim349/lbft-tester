/* conseil attendu : compte les mots en sautant */
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
char **ft_split(char const *s,char c){size_t n=1,i=0,j=0,k;char**t;for(k=0;s[k];k++)if(s[k]==c)n++;t=malloc((n+1)*sizeof(char*));if(!t)return 0;k=0;while(i<n){j=k;while(s[j]&&s[j]!=c)j++;t[i++]=ft_substr(s,k,j-k);k=j+1;}t[n]=0;return t;}

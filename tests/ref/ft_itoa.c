#include "libft.h"
#include <stdlib.h>
#include <unistd.h>
char *ft_itoa(int n){long v=n;int l=(n<=0);char*p;while(v){v/=10;l++;}p=malloc(l+1);if(!p)return 0;p[l]=0;v=n;if(v<0){p[0]=45;v=-v;}if(!v)p[0]=48;while(v){p[--l]=v%10+48;v/=10;}return p;}

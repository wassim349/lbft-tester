#ifndef FUZZ_H
# define FUZZ_H

void	fuzz_atoi(void);
void	fuzz_strlcpy(void);
void	fuzz_strlcat(void);
void	fuzz_strnstr(void);
void	fuzz_memmove(void);
void	fuzz_cmp(int which);
void	fuzz_chr(int which);
void	fuzz_itoa(void);
void	fuzz_substr(void);
void	fuzz_strtrim(void);
void	fuzz_strjoin(void);
void	fuzz_split(void);

#endif

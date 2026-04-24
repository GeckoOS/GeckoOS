#pragma once

#include <stdbool.h>
#include "stdint.h"

// for gk
#define is_digit(c) c >= '0' && c <= '9'
#define is_alpha(c) (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'
#define is_alnum(c) is_alpha(c) || is_digit(c)
#define is_space(c) c == ' ' || c == '\t'
// normal
#define isdigit(c) c >= '0' && c <= '9'
#define isalpha(c) (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'
#define isalnum(c) isalpha(c) || isdigit(c)
#define isspace(c) c == ' ' || c == '\t'

// defined in mem.c
uint8_t strcmp(const char *s, const char *d);
uint8_t strncmp(const char *s1, const char *s2, size_t siz);
int     strlen(const char* s);
void    strcat(char* buffer, const char* src);
void    strncat(char* buffer, const char* src, int n);
void    bzero(void* ptr, size_t siz);
void    memcpy(void *restrict dst, const void *restrict src, size_t n);
uint8_t memcmp(const void *s1, const void *s2, size_t n);
void    memset(void *dst, int c, size_t n);
void    strncpy(char *restrict dst, const char *restrict src, size_t dsize);
void    strcpy(char *restrict dst, const char *restrict src);
char   *strtok(char *restrict str, const char *restrict delim);
char   *strtok_r(char *restrict str, const char *restrict delim, char **restrict saveptr);
char   *strchr(const char *s, int c);
char   *strrchr(const char *s, int c);
char   *strdup(const char *s);
int     atoi(const char *str);
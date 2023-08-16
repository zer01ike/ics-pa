#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
    size_t i = 0;
    while (*s != '\0') {i++; s++;}
    return i;
}


char *strcpy(char *dst, const char *src) {
    char * ret = dst;
    while (*src != '\0')
    {
        *dst = *src;
        src++;
        dst++;
    }
    *dst = '\0';
    return ret;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i = 0;
  for (; i < n && src[i] != '\0'; i++) dst[i] = src[i];
  for (; i< n; i++) dst[i] = '\0';
  return dst;
}

char *strcat(char *dst, const char *src) {
  size_t dst_len = strlen(dst);
  size_t i = 0;
  while (src[i]!='\0')
  {
    dst[dst_len+i] = src[i];
    i++;
  }
  dst[dst_len+i] = '\0';
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  int i = 1;
  while (*s1 != '\0' && *s2 != '\0')
  {
    if (*s1 > *s2) return i;
    if (*s1 < *s2) return -i;
    s1++;
    s2++;
    i++; 
  }
  if (*s1 != '\0') return i;
  if (*s2 != '\0') return -i;
  return 0;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  int i = 1;
  while (i <= n && *s1 != '\0' && *s2 != '\0')
  {
    if (*s1 > *s2) return i;
    if (*s1 < *s2) return -i;
    s1++;
    s2++;
    i++; 
  }
  return 0;
}

void *memset(void *s, int c, size_t n) {
  void * ret = s;
  while (n--)
  {
    *(int *)s = c;
    s++;
  }
  return ret;
}

void *memmove(void *dst, const void *src, size_t n) {
  void * ret = dst;
  if (dst <=src || (char *)dst >= (char*)src + n)
  {
    while (n--)
    {
        *(char*)dst = *(char*)src;
        (char *)dst++;
        (char *)src++;
    }

    return ret;
  }

  dst = (char*)dst + n - 1;
  src = (char*)src + n - 1;
  while(n--)
  {
        *(char*)dst = *(char*)src;
        (char *)dst--;
        (char *)src--;
  }
  return ret;
}

void *memcpy(void *out, const void *in, size_t n) {
  return memmove(out, in, n);
}

int memcmp(const void *s1, const void *s2, size_t n) {
  return strncmp((const char *)s1, (const char *)s2, n);
}

#endif

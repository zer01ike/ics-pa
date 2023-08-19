#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  return 0;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
    char * s;
    int d;
    int ret = 0;
    while(*fmt)
    {
        if ((*fmt) != '%')
        {
            out[ret++] = *fmt;
            fmt++;
            continue;
        }
        fmt++;
        switch(*fmt++){
            case 's':
                {
                    s = va_arg(ap, char*);
                    size_t s_len = strlen(s);
                    size_t i = 0;
                    while (s_len--)
                    {
                        out[ret++] = s[i++];
                    }
                    continue;
                }
            case 'd':
                {
                    d = va_arg(ap, int);
                    int l = ret;
                    int r = l-1; 
                    while (d)
                    {
                        out[ret++] = d % 10 + '0';
                        d = d / 10;
                        r++;
                    }
                    while (l < r)
                    {
                       char t = out[l];
                       out[l] = out[r];
                       out[r] = t;
                       l++;
                       r--;
                    }
                    continue;
                }
        }
    }
    out[ret++] = '\0';
    return ret;
}

int sprintf(char *out, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsprintf(out, fmt, ap);
    va_end(ap);
    return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif

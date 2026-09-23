#include <stdio.h>
#include <stdarg.h>
#include <time.h>
void crash_log(const char *fmt, ...)
{
    FILE *f = fopen("emav_crash.log", "a");
    if (f) {
        va_list args;
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        va_end(args);
        fprintf(f, "\n");
        fflush(f);
        fclose(f);
    }
}

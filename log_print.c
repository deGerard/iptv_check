/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

extern int runasdaemon;
static FILE *logfile_fp = NULL;

int log_open(const char *logfile)
{
    logfile_fp = fopen(logfile, "w+");
    if (!logfile_fp) {
        return -1;
    }
    return 0;
}

void log_close(void)
{
    if (logfile_fp) {
        fclose(logfile_fp);
    }
    logfile_fp = NULL;
}


void log_printf (char *fmt, ...)
{
    va_list ap;
    char *textbuffer;
    int textlen;
    int textpos;

    if (!runasdaemon || logfile_fp) {
        textlen = 0;
        va_start(ap, fmt);
        textlen+=vsnprintf(NULL, 0, fmt, ap);
        va_end(ap);
        textlen+=8;
        textbuffer = malloc(textlen);
        if (!textbuffer) {
            return;
        }
        textpos = 0;
        va_start(ap, fmt);
        textpos+=vsnprintf(&textbuffer[textpos], textlen - textpos, fmt, ap);
        va_end(ap);
        textbuffer[textpos++] = 0;

        if (!runasdaemon) {
            printf("%s", textbuffer);
        }
        if (logfile_fp) {
            fprintf(logfile_fp, "%s", textbuffer);
            fflush(logfile_fp);
        }
        free(textbuffer);
    }
}

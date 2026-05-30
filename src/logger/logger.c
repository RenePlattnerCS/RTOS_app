#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "logger/logger.h"
#include "stm32f4xx.h"

LogLevel system_log_level = LOG_LEVEL_DEBUG;

#ifndef NDEBUG

static char const *_get_log_level_string(LogLevel const log_level)
{
    switch (log_level)
    {
        case LOG_LEVEL_ERROR:       return "ERROR";
        case LOG_LEVEL_INFORMATION: return "INFO";
        case LOG_LEVEL_DEBUG:       return "DEBUG";
        default:                    return "UNKNOWN";
    }
}

static void _log(LogLevel const log_level, char const *const format, va_list args)
{
    if (log_level > system_log_level)
        return;
    printf("[%s] ", _get_log_level_string(log_level));
    vfprintf(stdout, format, args);
    printf("\n");
}

void log_error(char const *const format, ...)
{
    va_list args;
    va_start(args, format);
    _log(LOG_LEVEL_ERROR, format, args);
    va_end(args);
}

void log_info(char const *const format, ...)
{
    va_list args;
    va_start(args, format);
    _log(LOG_LEVEL_INFORMATION, format, args);
    va_end(args);
}

void log_debug(char const *const format, ...)
{
    va_list args;
    va_start(args, format);
    _log(LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}

void log_debug_array(char const *const label, void const *array, uint16_t const len)
{
    if (LOG_LEVEL_DEBUG > system_log_level)
        return;
    printf("[%s] %s[%d]: {", _get_log_level_string(LOG_LEVEL_DEBUG), label, len);
    for (uint16_t i = 0; i < len; i++)
    {
        uint8_t val = *((uint8_t *)(array + i));
        printf("0x%02X", val);
        if (i < len - 1)
            printf(", ");
    }
    printf("}\n");
}

#endif /* NDEBUG */

// Syscall stubs — needed by the linker in both debug and release.
int _close(int file)   { (void)file; return -1; }
int _fstat(int file, void *st)  { (void)file; (void)st; return -1; }
int _isatty(int file)  { return (file == 1 || file == 2) ? 1 : 0; }
int _lseek(int file, int ptr, int dir) { (void)file; (void)ptr; (void)dir; return 0; }
int _read(int file, char *ptr, int len) { (void)file; (void)ptr; (void)len; return 0; }

#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>

typedef enum
{
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFORMATION,
    LOG_LEVEL_DEBUG
} LogLevel;

extern LogLevel system_log_level;

#ifndef NDEBUG

void log_error(char const *format, ...);
void log_info(char const *format, ...);
void log_debug(char const *format, ...);
void log_debug_array(char const *label, void const *array, uint16_t len);

#else

#define log_error(...)       ((void)0)
#define log_info(...)        ((void)0)
#define log_debug(...)       ((void)0)
#define log_debug_array(...) ((void)0)

#endif /* NDEBUG */

#endif /* LOGGER_H */

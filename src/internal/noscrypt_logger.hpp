#pragma once

#include <plog/Log.h>
#include <noscrypt.h>

/*
* @brief Logs an error message with the function name and line number where the 
* error occurred. This is useful for debugging and logging errors in the Noscrypt
* library.
*/
#define NC_LOG_ERROR(result) _printNoscryptError(result, __func__, __LINE__)

void _printNoscryptError(NCResult result, const char *func, int line);

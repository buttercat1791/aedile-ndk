#pragma once

#include <plog/Log.h>
#include <noscrypt.h>

/*
* @brief Logs an error message with the function name and line number where the 
* error occurred. This is useful for debugging and logging errors in the Noscrypt
* library.
*/
#define NC_LOG_ERROR(result) _printNoscryptError(result, __func__, __LINE__)

static void _printNoscryptError(NCResult result, const std::string funcName, int lineNum)
{
    uint8_t argPosition;

    switch (NCParseErrorCode(result, &argPosition))
    {
    case E_NULL_PTR:
        PLOG_ERROR << "noscrypt - error: A null pointer was passed in " << funcName << "(" << argPosition << ") at line " << lineNum;
        break;

    case E_INVALID_ARG:
        PLOG_ERROR << "noscrypt - error: An invalid argument was passed in " << funcName << "(" << argPosition << ") at line " << lineNum;
        break;

    case E_INVALID_CONTEXT:
        PLOG_ERROR << "noscrypt - error: An invalid context was passed in " << funcName << "(" << argPosition << ") on line " << lineNum;
        break;

    case E_ARGUMENT_OUT_OF_RANGE:
        PLOG_ERROR << "noscrypt - error: An argument was out of range in " << funcName << "(" << argPosition << ") at line " << lineNum;
        break;

    case E_OPERATION_FAILED:
        PLOG_ERROR << "noscrypt - error: An operation failed in " << funcName << "(" << argPosition << ") at line " << lineNum;
        break;

    default:
        PLOG_ERROR << "noscrypt - error: An unknown error " << result << " occurred in " << funcName << "(" << argPosition << ") at line " << lineNum;
        break;
    }
}

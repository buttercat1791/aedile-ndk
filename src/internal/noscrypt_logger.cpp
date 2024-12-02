#include "noscrypt_logger.hpp"

void _printNoscryptError(NCResult result, const char *func, int line)
{
    uint8_t argPosition;

    switch (NCParseErrorCode(result, &argPosition))
    {
    case E_NULL_PTR:
        PLOG_ERROR << "noscrypt - error: A null pointer was passed in " << func << "(" << argPosition << ") at line " << line;
        break;

    case E_INVALID_ARG:
        PLOG_ERROR << "noscrypt - error: An invalid argument was passed in " << func << "(" << argPosition << ") at line " << line;
        break;

    case E_INVALID_CONTEXT:
        PLOG_ERROR << "noscrypt - error: An invalid context was passed in " << func << "(" << argPosition << ") on line " << line;
        break;

    case E_ARGUMENT_OUT_OF_RANGE:
        PLOG_ERROR << "noscrypt - error: An argument was out of range in " << func << "(" << argPosition << ") at line " << line;
        break;

    case E_OPERATION_FAILED:
        PLOG_ERROR << "noscrypt - error: An operation failed in " << func << "(" << argPosition << ") at line " << line;
        break;

    default:
        PLOG_ERROR << "noscrypt - error: An unknown error " << result << " occurred in " << func << "(" << argPosition << ") at line " << line;
        break;
    }
}
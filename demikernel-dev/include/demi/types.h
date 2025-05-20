// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef DEMI_TYPES_H_IS_INCLUDED
#define DEMI_TYPES_H_IS_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <demi/cc.h>

#ifdef __linux__
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#ifdef _WIN32
#include <WinSock2.h>

// NB push the structure packing onto the stack with a label to ensure we correctly restore it at the end of the
// header.
#pragma pack(push, demi0)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Maximum number of segments in a scatter-gather array.
 */
#define DEMI_SGARRAY_MAXSIZE 1

    /**
     * @brief An I/O queue token.
     */
    typedef uint64_t demi_qtoken_t;

/**
 * @brief A segment of a scatter-gather array.
 */
#ifdef _WIN32
#pragma pack(push, 1)
    typedef struct demi_sgaseg
#endif
#ifdef __linux__
        typedef struct __attribute__((__packed__)) demi_sgaseg
#endif
    {
        void *sgaseg_buf;    /**< Underlying data.       */
        uint32_t sgaseg_len; /**< Size in bytes of data. */
    } demi_sgaseg_t;
#ifdef _WIN32
#pragma pack(pop)
#endif

/**
 * @brief A scatter-gather array.
 */
#ifdef _WIN32
#pragma pack(push, 1)
    typedef struct demi_sgarray
#endif
#ifdef __linux__
        typedef struct __attribute__((__packed__)) demi_sgarray
#endif
    {
        void *sga_buf;                                /**< Reserved.                                       */
        uint32_t sga_numsegs;                         /**< Number of segments in the scatter-gather array. */
        demi_sgaseg_t sga_segs[DEMI_SGARRAY_MAXSIZE]; /**< Scatter-gather array segments.                  */
        struct sockaddr_in sga_addr;                  /**< Source address of scatter-gather array.         */
    } demi_sgarray_t;
#ifdef _WIN32
#pragma pack(pop)
#endif

    /**
     * @brief Opcodes for an asynchronous I/O operation.
     */
    typedef enum demi_opcode
    {
        DEMI_OPC_INVALID = 0, /**< Invalid operation. */
        DEMI_OPC_PUSH,        /**< Push operation.    */
        DEMI_OPC_POP,         /**< Pop operation.     */
        DEMI_OPC_ACCEPT,      /**< Accept operation.  */
        DEMI_OPC_CONNECT,     /**< Connect operation. */
        DEMI_OPC_CLOSE,       /**< Close operation. */
        DEMI_OPC_FAILED,      /**< Operation failed.  */
    } demi_opcode_t;

/**
 * @brief Result value for an accept operation.
 */
#ifdef _WIN32
#pragma pack(push, 1)
    typedef struct demi_accept_result
#endif
#ifdef __linux__
        typedef struct __attribute__((__packed__)) demi_accept_result
#endif
    {
        int32_t qd;              /**< Socket I/O queue descriptor of accepted connection. */
        struct sockaddr_in addr; /**< Remote address of accepted connection.              */
    } demi_accept_result_t;
#ifdef _WIN32
#pragma pack(pop)
#endif

/**
 * @brief Result value for an asynchronous I/O operation.
 */
#ifdef _WIN32
#pragma pack(push, 1)
    typedef struct demi_qresult
#endif
#ifdef __linux__
        typedef struct __attribute__((__packed__)) demi_qresult
#endif
    {
        enum demi_opcode qr_opcode; /**< Opcode of completed operation.                              */
        int32_t qr_qd;              /**< I/O queue descriptor associated to the completed operation. */
        demi_qtoken_t qr_qt;        /**< I/O queue token of the completed operation.                 */
        int64_t qr_ret;             /**< Return code.                                                */

        /**
         * @brief Result value.
         */
        union
        {
            demi_sgarray_t sga;        /**< Pushed/popped scatter-gather array. */
            demi_accept_result_t ares; /**< Accept result.                      */
        } qr_value;
    } demi_qresult_t;
#ifdef _WIN32
#pragma pack(pop)
#endif

    // Callback Function.
    typedef void (*demi_callback_t)(const char *, uint32_t, uint64_t);

    // Log levels for demi_log_callback_t. These values correspond to the enum in flexi_logger crate.
    typedef enum demi_log_level
    {
        DemiLogLevel_Error = 1,
        DemiLogLevel_Warning = 2,
        DemiLogLevel_Info = 3,
        DemiLogLevel_Debug = 4,
        DemiLogLevel_Trace = 5,
    } demi_log_level_t;

    // Logging callback. Arguments are: level, module name, module length, file name, file name length, line number, message, message length, 
    typedef void (*demi_log_callback_t)(demi_log_level_t, const char*, uint32_t, const char*, uint32_t, uint32_t, const char*, uint32_t);

/**
 * @brief Arguments for Demikernel.
 */
#ifdef _WIN32
#pragma pack(push, 1)
    struct demi_args
#endif
#ifdef __linux__
        struct __attribute__((__packed__)) demi_args
#endif
    {
        int argc;                        /**< Number of command-line arguments. */
        char *const *argv;               /**< Command-line Arguments.           */
        demi_callback_t callback;        /**< Callback Function.                */
        demi_log_callback_t logCallback; /**< Logging Callback.                */
    };
#ifdef _WIN32
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#ifdef _WIN32
// Restore the original packing alignment.
#pragma pack(pop, demi0)
#endif

#endif /* DEMI_TYPES_H_IS_INCLUDED */

//  Copyright 2024 AntGroup CO., Ltd.
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//  http://www.apache.org/licenses/LICENSE-2.0
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

#pragma once
#include <exception>
#include <stdexcept>
#include <string>
#include <sstream>
#include "fma-common/string_formatter.h"

namespace lgraph_api {
enum class ErrorCode {
    OK = 0,
    InvalidArguments,
    InvalidGalaxy,
    InvalidGraphDB,
    InvalidTransaction,
    InvalidIterator,
    InvalidTransactionFork,
    TaskKilled,
    TransactionConflict,
    WriteNotAllowed,
    DBNotExist,
    IOError,
    Unauthorized,
    InternalError,
    BadRequest,
    LmdbException,
    FullTextIndexException,
    PythonPluginTimeout,
    InputError,
    OutOfRange,
    LgraphPeekError,
};

inline const char* ToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::OK: return "Ok";
        case ErrorCode::InvalidArguments: return "InvalidArguments";
        case ErrorCode::InvalidGalaxy: return "InvalidGalaxy";
        case ErrorCode::InvalidGraphDB: return "InvalidGraphDB";
        case ErrorCode::InvalidTransaction: return "InvalidTransaction";
        case ErrorCode::InvalidIterator: return "InvalidIterator";
        case ErrorCode::InvalidTransactionFork: return "InvalidTransactionFork";
        case ErrorCode::TaskKilled: return "TaskKilled";
        case ErrorCode::TransactionConflict: return "TransactionConflict";
        case ErrorCode::WriteNotAllowed: return "WriteNotAllowed";
        case ErrorCode::DBNotExist: return "DBNotExist";
        case ErrorCode::IOError: return "IOError";
        case ErrorCode::Unauthorized: return "Unauthorized";
        case ErrorCode::InternalError: return "InternalError";
        case ErrorCode::BadRequest: return "BadRequest";
        case ErrorCode::LmdbException: return "LmdbException";
        case ErrorCode::FullTextIndexException: return "FullTextIndexException";
        case ErrorCode::PythonPluginTimeout: return "PythonPluginTimeout";
        case ErrorCode::InputError: return "InputError";
        case ErrorCode::OutOfRange: return "OutOfRange";
        case ErrorCode::LgraphPeekError: return "LgraphPeekError";
        default: return "UnknownErrorCode";
    }
}

class LgraphException : public std::exception {
 public:
    explicit LgraphException(ErrorCode code, const std::string& msg)
        : code_(code), msg_(msg) {
        what_ = FMA_FMT("[{}] {}", ToString(code_), msg_);
    }
    explicit LgraphException(ErrorCode code, const char* msg)
        : code_(code), msg_(msg) {}
    ErrorCode code() {return code_;};
    const std::string& msg() {return msg_;};
    const char* what() const noexcept override {
        return what_.c_str();
    };
 private:
    ErrorCode code_;
    std::string msg_;
    std::string what_;
};

#define DEFINE_EXCEPTION(name, default_msg)                           \
class name : public LgraphException {                                 \
 public:                                                              \
    name()                                                            \
        : LgraphException(ErrorCode::name, default_msg) {}            \
    explicit name(const std::string& err)                             \
        : LgraphException(ErrorCode::name, err) {}                    \
    explicit name(const char* err)                                    \
        : LgraphException(ErrorCode::name, err) {}                    \
    template <typename... Ts>                                         \
    name(const char* format, const Ts&... ds)                         \
        : LgraphException(ErrorCode::name, FMA_FMT(format, ds...)) {} \
}

/** @brief   User input error. Base class for a variety of input errors. */
DEFINE_EXCEPTION(InputError, "InputError");
/** @brief   Data out of range. */
DEFINE_EXCEPTION(OutOfRange, "OutOfRange");
/** @brief   An invalid parameter is passed. */
DEFINE_EXCEPTION(InvalidArguments, "Invalid Arguments");
/** @brief   Function called on an invalid Galaxy. */
DEFINE_EXCEPTION(InvalidGalaxy, "Invalid Galaxy.");
/** @brief   Function called on an invalid GraphDB. */
DEFINE_EXCEPTION(InvalidGraphDB, "Invalid GraphDB.");
/** @brief   Function called on an invalid transaction. */
DEFINE_EXCEPTION(InvalidTransaction, "Invalid transaction.");
/** @brief   Function called on an invalid iterator. */
DEFINE_EXCEPTION(InvalidIterator, "Invalid iterator.");
/** @brief   ForkTxn called on a write transaction. */
DEFINE_EXCEPTION(InvalidTransactionFork, "Write transactions cannot be forked.");
/** @brief   Task is being killed per user request or timeout. */
DEFINE_EXCEPTION(TaskKilled, "Task is killed.");
/** @brief   A conflict is detected when committing this transaction. */
DEFINE_EXCEPTION(TransactionConflict, "Transaction conflicts with an earlier one.");
/** @brief   Write operation is tried on a read-only GraphDB or read-only transaction. */
DEFINE_EXCEPTION(WriteNotAllowed, "Write operation is not allowed.");
/** @brief   Specified database does not exist, wrong directory? */
DEFINE_EXCEPTION(DBNotExist, "The specified TuGraph DB does not exist.");
/** @brief   An i/o error. */
DEFINE_EXCEPTION(IOError, "IO Error.");
/** @brief   User not authorized to perform this action. */
DEFINE_EXCEPTION(Unauthorized, "Unauthorized.");
DEFINE_EXCEPTION(InternalError, "Internal error.");
DEFINE_EXCEPTION(BadRequest, "Bad request.");
DEFINE_EXCEPTION(PythonPluginTimeout, "Python plugin timeout.");
DEFINE_EXCEPTION(LgraphPeekError, "LgraphPeekError.");

}  // namespace lgraph_api

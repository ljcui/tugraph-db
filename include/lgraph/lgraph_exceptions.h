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
};

class LgraphException : public std::exception {
 public:
    explicit LgraphException(ErrorCode code, const std::string& msg)
        : code_(code), msg_(msg) {}
    explicit LgraphException(ErrorCode code, const char* msg)
        : code_(code), msg_(msg) {}
    ErrorCode code() {return code_;};
    const std::string& msg() {return msg_;};
    const char* what() const noexcept override {
        return msg_.c_str();
    };
 private:
    ErrorCode code_;
    std::string msg_;
};

/** @brief   User input error. Base class for a variety of input errors. */
class InputError : public LgraphException {
 public:
    explicit InputError(const std::string& err)
        : LgraphException(ErrorCode::InputError, err) {}
    explicit InputError(const char* err)
        : LgraphException(ErrorCode::InputError, err) {}
    template <typename... Ts>
    InputError(const char* format, const Ts&... ds)
        : LgraphException(ErrorCode::InputError, FMA_FMT(format, ds...)) {}
};

/** @brief   Data out of range. */
class OutOfRangeError : public std::range_error {
 public:
    explicit OutOfRangeError(const std::string& err) : std::range_error(err) {}
};

/** @brief   An invalid parameter is passed. */
class InvalidArguments : public LgraphException {
 public:
    explicit InvalidArguments(const std::string& msg = "Invalid Arguments.")
        : LgraphException(ErrorCode::InvalidArguments, msg) {}
};

/** @brief   Function called on an invalid Galaxy. */
class InvalidGalaxy : public LgraphException {
 public:
    InvalidGalaxy() : LgraphException(ErrorCode::InvalidGalaxy, "Invalid Galaxy.") {}
};

/** @brief   Function called on an invalid GraphDB. */
class InvalidGraphDB : public LgraphException {
 public:
    InvalidGraphDB() : LgraphException(ErrorCode::InvalidGraphDB, "Invalid GraphDB.") {}
};

/** @brief   Function called on an invalid transaction. */
class InvalidTransaction : public LgraphException {
 public:
    InvalidTransaction() : LgraphException(ErrorCode::InvalidTransaction, "Invalid transaction.") {}
};

/** @brief   Function called on an invalid iterator. */
class InvalidIterator : public LgraphException {
 public:
    InvalidIterator() : LgraphException(ErrorCode::InvalidIterator, "Invalid iterator.") {}
};

/** @brief   ForkTxn called on a write transaction. */
class InvalidTransactionFork : public LgraphException {
 public:
    InvalidTransactionFork() : LgraphException(ErrorCode::InvalidTransactionFork, "Write transactions cannot be forked.") {}
};

/** @brief   Task is being killed per user request or timeout. */
class TaskKilled : public LgraphException {
 public:
    TaskKilled() : LgraphException(ErrorCode::TaskKilled, "Task is killed.") {}
};

/** @brief   A conflict is detected when committing this transaction. */
class TransactionConflict : public LgraphException {
 public:
    TransactionConflict() : LgraphException(ErrorCode::TransactionConflict,
                                            "Transaction conflicts with an earlier one.") {}
};

/** @brief   Write operation is tried on a read-only GraphDB or read-only transaction. */
class WriteNotAllowed : public LgraphException {
 public:
    explicit WriteNotAllowed(const std::string& msg = "Write operation is not allowed.")
        : LgraphException(ErrorCode::WriteNotAllowed, msg) {}
};

/** @brief   Specified database does not exist, wrong directory? */
class DBNotExist : public LgraphException {
 public:
    explicit DBNotExist(const std::string& msg = "The specified TuGraph DB does not exist.")
        : LgraphException(ErrorCode::DBNotExist, msg) {}
};

/** @brief   An i/o error. */
class IOError : public LgraphException {
 public:
    explicit IOError(const std::string& msg = "IO Error.") : LgraphException(ErrorCode::IOError, msg) {}
};

/** @brief   User not authorized to perform this action. */
class Unauthorized : public LgraphException {
 public:
    explicit Unauthorized(const std::string& msg = "Unauthorized.")
        : LgraphException(ErrorCode::Unauthorized, msg) {}
};

class InternalError : public LgraphException {
 public:
    explicit InternalError(const std::string& err)
        : LgraphException(ErrorCode::InternalError, err) {}
};

class BadRequest : public LgraphException {
 public:
    explicit BadRequest(const std::string& err) : LgraphException(ErrorCode::BadRequest, err) {}
};

class PythonPluginTimeout : public LgraphException {
 public:
    explicit PythonPluginTimeout(const std::string& err)
        : LgraphException(ErrorCode::PythonPluginTimeout, err) {}
};

}  // namespace lgraph_api

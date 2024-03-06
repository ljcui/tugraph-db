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
#define ERROR_CODES \
X(InvalidArguments, "InvalidArguments") \
X(InvalidGalaxy, "InvalidGalaxy")   \
X(InvalidGraphDB, "InvalidGraphDB")   \
X(InvalidTransaction, "InvalidTransaction") \
X(InvalidIterator, "InvalidIterator")   \
X(InvalidTransactionFork, "InvalidTransactionFork") \
X(TaskKilled, "TaskKilled")   \
X(TransactionConflict, "TransactionConflict")  \
X(WriteNotAllowed, "WriteNotAllowed")   \
X(DBNotExist, "DBNotExist")  \
X(IOError, "IOError")  \
X(Unauthorized, "Unauthorized")  \
X(InternalError, "InternalError")  \
X(BadRequest, "BadRequest")   \
X(LmdbException, "LmdbException")  \
X(FullTextIndexException, "FullTextIndexException")   \
X(PythonPluginTimeout, "PythonPluginTimeout")  \
X(InputError, "InputError")   \
X(OutOfRange, "OutOfRange")   \
X(LgraphPeekError, "LgraphPeekError")   \
X(LgraphExportError, "LgraphExportError")  \
X(LgraphBinlogError, "LgraphBinlogError")   \
X(LgraphCliError, "LgraphCliError")  \
X(OlapOnDiskError, "OlapOnDiskError")   \
X(OlapOnDBError, "OlapOnDBError")   \
X(OlapError, "OlapError")   \
X(RestServerError, "RestServerError")  \
X(MismatchedMajorVersion, "MismatchedMajorVersion")   \
X(Filesystem, "Filesystem")  \
X(CorrupttedDB, "CorrupttedDB")         \
X(CorrupttedData, "CorrupttedData")         \
X(CorrupttedDbConfig, "CorrupttedDbConfig")  \
X(PythonPluginError, "PythonPluginError")   \
X(PluginError, "PluginError")   \
X(RestClientError, "RestClientError")   \
X(PythonApiError, "PythonApiError")  \
X(ConvertError, "ConvertError")         \
X(Unsupported, "Unsupported")           \
X(Unexpected, "Unsupported")           \
X(UnsupportedType, "UnsupportedType")   \
X(LargeString, "String size too large") \
X(UnsupportedPointer, "UnsupportedPointer") \
X(AddIndexError, "AddIndexError")       \
X(OnlineImportError, "OnlineImportError")   \
X(UnknownSpatialType, "UnknownSpatialType") \
X(UnknownType, "UnknownType")           \
X(BadCast, "BadCast")                   \
X(Invalidname, "Invalidname")           \
X(InvalidUserNum, "InvalidUserNum")     \
X(InvalidRoleNum, "InvalidRoleNum")     \
X(InvalidGraphNum, "InvalidGraphNum")   \
X(InvalidLabelNum, "InvalidLabelNum")   \
X(InvalidFieldNum, "InvalidFieldNum")   \
X(InvalidDesc, "InvalidDesc")  \
X(InvalidGraphSize, "InvalidGraphSize") \
X(InvalidInvalidPassword, "InvalidInvalidPassword") \
X(TimeZoneError, "TimeZoneError")       \
X(MaxVidReached, "MaxVidReached") \
X(AuditLogError, "AuditLogError") \
X(MaxEidReached, "MaxEidReached") \
X(MmapError, "MmapError")    \
X(MallocError, "MallocError")  \
X(TraversalError, "TraversalError") \
X(BoltError, "BoltError") \
X(NoVertexFoundByIndex, "NoVertexFoundByIndex")     \
X(NoEdgeFoundByIndex, "NoEdgeFoundByIndex") \
X(ParallelBitsetError, "ParallelBitsetError") \
X(ResultRecordError, "ResultRecordError")   \
X(HaStateMachineError, "HaStateMachineError") \
X(DBManagementClientError, "DBManagementClientError") \
X(HaConfigError, "HaConfigError")       \
X(UnknownSymbolNodeScope, "UnknownSymbolNodeScope") \
X(CypherPathError, "CypherPathError") \
X(ImportV3Error, "ImportV3Error")      \
X(ImportV2Error, "ImportV2Error")      \
X(ImportConfigError, "ImportConfigError") \
X(FileCutError, "FileCutError")         \
X(ColumnParserError, "ColumnParserError")

enum class ErrorCode {
#define X(code, msg) code,
    ERROR_CODES
#undef X
};

inline const char* ErrorCodeToString(ErrorCode code) {
    switch (code) {
#define X(code, msg) case ErrorCode::code: return #code;
        ERROR_CODES
#undef X
    default: return "Unknown Error Code";
    }
}

inline const char* ErrorCodeDesc(ErrorCode code) {
    switch (code) {
#define X(code, msg) case ErrorCode::code: return msg;
        ERROR_CODES
#undef X
    default: return "Unknown Error Code";
    }
}

class LgraphException : public std::exception {
 public:
    explicit LgraphException(ErrorCode code)
        : code_(code), msg_(ErrorCodeDesc(code)) {
        what_ = FMA_FMT("[{}] {}", ErrorCodeToString(code_), msg_);
    }
    explicit LgraphException(ErrorCode code, const std::string& msg)
        : code_(code), msg_(msg) {
        what_ = FMA_FMT("[{}] {}", ErrorCodeToString(code_), msg_);
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

#define DEFINE_EXCEPTION(name)                                        \
class name : public LgraphException {                                 \
 public:                                                              \
    name()                                                            \
        : LgraphException(ErrorCode::name) {}                         \
    explicit name(const std::string& err)                             \
        : LgraphException(ErrorCode::name, err) {}                    \
    explicit name(const char* err)                                    \
        : LgraphException(ErrorCode::name, err) {}                    \
    template <typename... Ts>                                         \
    explicit name(const char* format, const Ts&... ds)                \
        : LgraphException(ErrorCode::name, FMA_FMT(format, ds...)) {} \
}

#define X(code, msg) DEFINE_EXCEPTION(code);
ERROR_CODES
#undef X

}  // namespace lgraph_api

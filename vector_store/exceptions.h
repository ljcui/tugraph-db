#pragma once

#include <exception>
#include <stdexcept>
#include <string>
#include <sstream>
#include <spdlog/fmt/fmt.h>

#define ERROR_CODES \
X(UnknownError, "Unknown Error.") \
X(VectorIndexException, "Vector index exception.")

enum class ErrorCode {
#define X(code, msg) code,
    ERROR_CODES
#undef X
};

const char* ErrorCodeToString(ErrorCode code);
const char* ErrorCodeDesc(ErrorCode code);

class VectorIndexException : public std::exception {
public:
    explicit VectorIndexException(ErrorCode code);
    explicit VectorIndexException(ErrorCode code, std::string msg);
    explicit VectorIndexException(ErrorCode code, const char* msg);
    template <typename... Ts>
    explicit VectorIndexException(ErrorCode code, const char* format, const Ts&... ds)
            : code_(code), msg_(fmt::format(format, ds...)) {
        what_ = fmt::format("[{}] {}", ErrorCodeToString(code_), msg_);
    }
    [[nodiscard]] ErrorCode code() const {return code_;}
    [[nodiscard]] const std::string& msg() const {return msg_;}
    [[nodiscard]] const char* what() const noexcept override {
        return what_.c_str();
    };
private:
    ErrorCode code_;
    std::string msg_;
    std::string what_;
};

#define THROW_CODE(code, ...) throw VectorIndexException(ErrorCode::code, ##__VA_ARGS__)
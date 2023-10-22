//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/26.
//

#pragma once

#include <sstream>
#include <string>
#include <functional>
#include <string_view>
#include <iostream>
#include <type_traits>
#include <utility>
#include <ctime>
#include "absl/strings/substitute.h"

#include "sight_util.h"

namespace sight {

#ifdef SIGHT_DEBUG
    extern bool logSourceLocation;
#endif

    enum class NodePortType;

    enum class LogLevel {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
    };

    /**
     * @brief 
     * 
     * @param func 
     * @return true success
     * @return false fail
     */
    bool registerLogWriter(std::function<void(LogLevel, std::string_view msg)> func);

    bool unregisterLogWriter();

    void callLogWriter(LogLevel, const char* msg);

    class LogConstructor {
        // std::string buffer{};
        std::ostringstream buffer{};
        LogLevel l;

    public:
        LogConstructor(const char* filename, int lineno, const char* functionName, LogLevel l)
            : l(l) {
            // append time
            std::time_t t = std::time(0);     // get time now
            std::tm* now = std::localtime(&t);
            buffer << '[';
            if (now->tm_hour < 10) {
                buffer << '0' << now->tm_hour;
            } else {
                buffer << now->tm_hour;
            }
            buffer << ':';
            if (now->tm_min < 10) {
                buffer << '0' << now->tm_min;
            } else {
                buffer << now->tm_min;
            }
            buffer << ':';
            if (now->tm_sec < 10) {
                buffer << '0' << now->tm_sec;
            } else {
                buffer << now->tm_sec;
            }
            buffer << "] ";

#ifdef SIGHT_DEBUG
            if (logSourceLocation) {
                buffer << absl::Substitute("[$0:$1 ($2)] ", filename, lineno, functionName);
            }
#endif

            // switch (l) {
            // case LogLevel::Trace:
            //     buffer << "[TRACE] ";
            //     break;
            // case LogLevel::Debug:
            //     buffer << "[DEBUG] ";
            //     break;
            // case LogLevel::Info:
            //     buffer << "[INFO] ";
            //     break;
            // case LogLevel::Warning:
            //     buffer << "[WARNING] ";
            //     break;
            // case LogLevel::Error:
            //     buffer << "[ERROR] ";
            //     break;
            // }
        }

        void nextAndWrite(std::string_view str) {
            buffer << str ;
            auto tmp = buffer.str();
            std::cout << tmp << std::endl;
            
            callLogWriter(this->l, tmp.c_str());
        }

        template<typename T>
        std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())> print(T u) {
            buffer << u;
            std::string msg = buffer.str();
            std::cout << msg << std::endl;

            callLogWriter(this->l, msg.c_str());
        }

        template<typename... Args>
        void print(const char* fmt, Args&&... args) {
            nextAndWrite(printValues(fmt, std::forward<Args>(args)...));
        }

        template<typename... Args>
        void print(const wchar_t* fmt, Args&&... args) {
            nextAndWrite(printValues(fmt, std::forward<Args>(args)...));
        }

        absl::substitute_internal::Arg convert(sight::NodePortType type) {
            return absl::substitute_internal::Arg(static_cast<int>(type));
        }

        template<typename T>
        auto convertIfNodePortType(T&& arg) {
            if constexpr (std::is_same_v<std::decay_t<T>, sight::NodePortType>) {
                return convert(arg);
            } else {
                return std::forward<T>(arg);
            }
        }

        template<typename... Args>
        std::string printValues(const char* fmt, Args&&... args) {
            return absl::Substitute(fmt, convertIfNodePortType(std::forward<Args>(args))...);
        }

        template<typename... Args>
        std::string printValues(const wchar_t* fmt, Args&&... args) {
            auto str = narrow(fmt);
            return absl::Substitute(str.c_str(), convertIfNodePortType(std::forward<Args>(args))...);
        }
    };

}

#ifdef SIGHT_DEBUG
#    define trace(...) sight::LogConstructor(__FILE__, __LINE__, __func__, LogLevel::Trace).print(__VA_ARGS__)
#    define logDebug(...) sight::LogConstructor(__FILE__, __LINE__, __func__, LogLevel::Debug).print(__VA_ARGS__)
#    define logInfo(...) sight::LogConstructor(__FILE__, __LINE__, __func__, LogLevel::Info).print(__VA_ARGS__)
#    define logWarning(...) sight::LogConstructor(__FILE__, __LINE__, __func__, LogLevel::Warning).print(__VA_ARGS__)
#    define logError(...) sight::LogConstructor(__FILE__, __LINE__, __func__, LogLevel::Error).print(__VA_ARGS__)
#else
#    define trace(...)
#    define logDebug(...)
#    define logInfo(...) sight::LogConstructor("", 0, "", LogLevel::Info).print(__VA_ARGS__)
#    define logWarning(...) sight::LogConstructor("", 0, "", LogLevel::Warning).print(__VA_ARGS__)
#    define logError(...) sight::LogConstructor("", 0, "", LogLevel::Info).Error(__VA_ARGS__)
#endif

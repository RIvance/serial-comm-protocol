
#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>

namespace logger
{
    using std::string;

    namespace level
    {
        enum LogLevel : unsigned char
        {
            NONE    =   0,
            ERROR   =   1,
            WARNING =   2,
            INFO    =   3,
            DEBUG   =   4,
        };

        static LogLevel level = INFO;
    }

    using level::LogLevel;

    static inline void setLogLevel(LogLevel lvl)
    {
        logger::level::level = lvl;
    }

    namespace format
    {
        using namespace std::chrono;
        using SysClock = std::chrono::system_clock;

        static inline void printErrLn() { std::cerr << '\n'; }

        template<typename T, typename ... Ts>
        static inline void printErrLn(const T & first, Ts ... tail)
        {
            std::cerr << first;
            printErrLn(tail...);
        }

      #define COLOR_DEF(color, code)  static const char* color = "\033[0;" #code "m"

        COLOR_DEF(RED,      31);
        COLOR_DEF(GREEN,    32);
        COLOR_DEF(YELLOW,   33);
        COLOR_DEF(BLUE,     34);
        COLOR_DEF(CYAN,     36);

      #undef COLOR_DEF

        static inline string colorString(const string & s, const char* color)
        {
            return color + s + "\033[0m";
        }

        static inline string timeString()
        {
            std::stringstream timeFormat;
            char buffer[64];
            auto now = SysClock::now();
            long millisecond = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;
            timeFormat << "%Y-%m-%d %H:%M:%S." << std::setw(3) << std::setfill('0') << millisecond;
            time_t timeNow = SysClock::to_time_t(now);
            std::strftime(buffer, sizeof(buffer), timeFormat.str().c_str(), std::localtime(&timeNow));
            return buffer;
        }
    }

    template<typename ... Ts>
    static inline void log(const string & label, const char* color, Ts ... args)
    {
        std::cerr << format::colorString("[", format::BLUE);
        std::cerr << format::colorString(label, color);
        std::cerr << format::timeString() << " ";
        std::cerr << format::colorString("]", format::BLUE) << " ";
        format::printErrLn(args...);
    }

    template<typename ... Ts>
    static inline void debug(Ts ... args)
    {
        if (logger::level::level >= logger::level::DEBUG) {
            logger::log(" DEBUG ", format::CYAN, args...);
        }
    }

    template<typename ... Ts>
    static inline void info(Ts ... args)
    {
        if (logger::level::level >= logger::level::INFO) {
            logger::log(" INFO  ", format::GREEN, args...);
        }
    }

    template<typename ... Ts>
    static inline void warning(Ts ... args)
    {
        if (logger::level::level >= logger::level::WARNING) {
            logger::log(" WARN  ", format::YELLOW, args...);
        }
    }

    template<typename ... Ts>
    static inline void error(Ts ... args)
    {
        if (logger::level::level >= logger::level::ERROR) {
            logger::log(" ERROR ", format::RED, args...);
        }
    }
}

#endif // LOGGER_HPP

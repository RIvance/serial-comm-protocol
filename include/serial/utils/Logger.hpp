
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
        enum LogLevel : uint8_t
        {
            LOG_NONE    =   0,
            LOG_ERROR   =   1,
            LOG_WARNING =   2,
            LOG_INFO    =   3,
            LOG_DEBUG   =   4,
        };

        static LogLevel level = LOG_DEBUG;
    }

    using level::LogLevel;

    static inline void setLogLevel(LogLevel lvl)
    {
        logger::level::level = lvl;
    }

    namespace io
    {
        using namespace std::chrono;
        using SysClock = std::chrono::system_clock;
        using StrStream = std::stringstream;

        static inline void printErrLn(StrStream & stream)
        {
            std::cerr << stream.str() << '\n';
        }

        template<typename T, typename ... Ts>
        static void printErrLn(StrStream & stream, const T & first, const Ts &... tail)
        {
            stream << first;
            printErrLn(stream, tail...);
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
            return string(color) + s + "\033[0m";
        }

        static inline string timeString()
        {
            StrStream timeFormat;
            char buffer[64];
            auto now = SysClock::now();
            long millisecond = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;
            timeFormat << "%Y-%m-%d %H:%M:%S." << std::setw(3) << std::setfill('0') << millisecond;
            time_t timeNow = SysClock::to_time_t(now);
            std::strftime(buffer, sizeof(buffer), timeFormat.str().c_str(), std::localtime(&timeNow));
            return buffer;
        }

        template<typename ... Ts>
        static inline void log(const string & label, const char* color, const Ts &... args)
        {
            StrStream stream;
            stream << io::colorString("[", io::BLUE);
            stream << io::colorString(label, color);
          #ifdef _GLIBCXX_THREAD
            stream << "T" << std::this_thread::get_id() << " ";
          #endif
            stream << io::timeString() << " ";
            stream << io::colorString("]", io::BLUE) << " ";
            io::printErrLn(stream, args...);
        }
    }

    template<typename ... Ts>
    static inline void debug(const Ts &... args)
    {
        if (logger::level::level >= logger::level::LOG_DEBUG) {
            logger::io::log(" DEBUG ", io::CYAN, args...);
        }
    }

    template<typename ... Ts>
    static inline void info(const Ts &... args)
    {
        if (logger::level::level >= logger::level::LOG_INFO) {
            logger::io::log(" INFO  ", io::GREEN, args...);
        }
    }

    template<typename ... Ts>
    static inline void warning(const Ts &... args)
    {
        if (logger::level::level >= logger::level::LOG_WARNING) {
            logger::io::log(" WARN  ", io::YELLOW, args...);
        }
    }

    template<typename ... Ts>
    static inline void error(const Ts &... args)
    {
        if (logger::level::level >= logger::level::LOG_ERROR) {
            logger::io::log(" ERROR ", io::RED, args...);
        }
    }
}

#endif // LOGGER_HPP

#ifndef __LOGGER_H
#define __LOGGER_H


#include <vector>
#include <memory>
#include <mutex>
#include <map>
#include <syslog.h>


namespace logging {


enum class Loglevel {
    ERROR   = LOG_ERR,
    WARNING = LOG_WARNING,
    INFO    = LOG_INFO,
    DEBUG   = LOG_DEBUG
};

// Loglevel string representation
const std::map<Loglevel, std::string> loglevel_str = {
    {Loglevel::ERROR,     "ERROR"},
    {Loglevel::WARNING,   "WARNING"},
    {Loglevel::INFO,      "INFO"},
    {Loglevel::DEBUG,     "DEBUG"}
};

/*
 * Represents a Logger Sink (output) base class.
 * It is an abstract class, so that it can't be instantiated, just inherited.
 */
class Sink {
public:
    Sink(Loglevel lvl):
        level(lvl)
    { }

    void set_level(Loglevel lvl);

    /*
     * Formats a message like: "[$LOGLEVEL] $DATE $MESSAGE".
     * Passes messages with an apropriate loglevel to write abstract method.
     * Should never be called directly, only used by Logger object.
     * params:
     *      lvl - message loglevel
     *      msg - message string
     */
    void push(Loglevel lvl, const std::string& msg);

protected:
    Loglevel level;

    /*
     * Abstract method writing message. Should be overloaded by an inherited class.
     */
    virtual void write(const std::string& msg) = 0;
};

/*
 * Represents Syslog Sink class. Writes Logger messages to linux syslog.
 */
class SyslogSink: public Sink {
public:
    SyslogSink(Loglevel lvl);

   ~SyslogSink();

private:
    virtual void write(const std::string& msg);
};

/*
 * Represents Console Sink class. Writes Logger messages to the console.
 */
class ConsoleSink: public Sink {
public:
    ConsoleSink(Loglevel lvl):
        Sink(lvl)
    { }

private:
    virtual void write(const std::string& msg);
};


/*
 * Represents a simple logger. Logger should never be instantiated directly,
 * but always through get_instance static function.
 * Single for the entire application.
 * Thread-safe.
 */
class Logger {
public:
    Logger(const Logger& other) = delete;
    Logger& operator=(const Logger& other) = delete;

    /*
     * Returns the application Logger object pointer
     */
    static Logger* get_instance();

    /*
     * Adds a Sink to Logger
     * params:
     *      sink_ptr - Sink object shared pointer to be added to Logger
     */
    void add_sink(const std::shared_ptr<Sink>& sink_ptr);

    /*
     * Deletes a Sink from Logger
     * param:
     *      sink_ptr - Sink object shared pointer to be deleted from Logger
     */
    void del_sink(const std::shared_ptr<Sink>& sink_ptr);

    /*
     * Logs the message msg with a loglevel lvl
     * params:
     *      lvl - loglevel
     *      msg - log message
     */
    void log(Loglevel lvl, const std::string& msg);

    /*
     * Logs a message msg with a debug loglevel
     * params:
     *      msg - log message
     */
    void debug(const std::string& msg);

    /*
     * Logs a message msg with a info loglevel
     * params:
     *      msg - log message
     */
    void info(const std::string& msg);

    /*
     * Logs a message msg with a error loglevel
     * params:
     *      msg - log message
     */
    void error(const std::string& msg);

    /*
     * Logs a message msg with a warning loglevel
     * params:
     *      msg - log message
     */
    void warning(const std::string& msg);

private:
    static Logger logger;                       // static logger object implementing a singleton pattern
    std::vector<std::shared_ptr<Sink>> sinks;   // logger sinks (outputs) list
    mutable std::mutex mx;                      // mutex for thread-safe support

    Logger()    // hides constructor to prevent direct instantiation
    { }
};


} // namespace logging


#endif // __LOGGER_H
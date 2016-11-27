#include <logger.h>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <memory>
#include <mutex>
#include <map>
#include <syslog.h>


namespace logging {


void Sink::set_level(Loglevel lvl)
{
    level = lvl;
}

void Sink::push(Loglevel lvl, const std::string& msg)
{
    if (level >= lvl) {
        std::time_t t = std::time(nullptr);
        std::stringstream out;

        out << std::left
            << std::setw(10)
            << "[" + loglevel_str.at(lvl) + "]"
            << std::put_time(std::localtime(&t), "%D %T ")
            << msg;

        write(out.str());
    }
}


SyslogSink::SyslogSink(Loglevel lvl):
    Sink(lvl)
{
    openlog(NULL, 0, LOG_USER);
}

SyslogSink::~SyslogSink()
{
    closelog();
}

void SyslogSink::write(const std::string& msg)
{
    syslog(static_cast<int>(level), "%s", msg.c_str());
}


void ConsoleSink::write(const std::string& msg)
{
    std::cout << msg << std::endl;
}


Logger* Logger::get_instance()
{
    return &logger;
}

void Logger::add_sink(const std::shared_ptr<Sink>& sink_ptr)
{
    std::lock_guard<std::mutex> lk(mx);
    sinks.push_back(sink_ptr);
}
void Logger::del_sink(const std::shared_ptr<Sink>& sink_ptr)
{
    std::lock_guard<std::mutex> lk(mx);
    std::remove_if(sinks.begin(), sinks.end(), [&sink_ptr] (const std::shared_ptr<Sink>& sp) {
        return sp.get() == sink_ptr.get();
    });
}

void Logger::log(Loglevel lvl, const std::string& msg)
{
    std::lock_guard<std::mutex> lk(mx);
    for (auto sink_ptr: sinks) {
        sink_ptr->push(lvl, msg);
    }
}

void Logger::debug(const std::string& msg)
{
    log(Loglevel::DEBUG, msg);
}

void Logger::info(const std::string& msg)
{
    log(Loglevel::INFO, msg);
}

void Logger::error(const std::string& msg)
{
    log(Loglevel::ERROR, msg);
}

void Logger::warning(const std::string& msg)
{
    log(Loglevel::WARNING, msg);
}

Logger Logger::logger;


} // namespace logging
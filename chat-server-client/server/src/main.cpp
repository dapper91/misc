#include <stdint.h>
#include <string>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/format.hpp>

#include <server.h>
#include <logger.h>


using namespace logging;
namespace popt = boost::program_options;


struct Arguments {
    std::string iface;
    uint16_t port;
};


Arguments parse_args(int argc, char** argv)
{
    Arguments args;

    popt::options_description options("Options");
    options.add_options()
            ("help,h", "show help")
            ("iface,i", popt::value<std::string>()->required(), "interface to listen on")
            ("port,p", popt::value<uint16_t>()->required(), "port to listen on");

    popt::variables_map vm;

    try {
        popt::store(popt::parse_command_line(argc, argv, options), vm);

        if (vm.count("help")) {
            std::cout << boost::format("Usage: %1% -i IFACE -p PORT") % argv[0] << std::endl;
            std::cout << options << std::endl;
            std::exit(0);
        }

        popt::notify(vm);
        args.iface = vm["iface"].as<std::string>();
        args.port = vm["port"].as<uint16_t>();
    }
    catch(popt::error& e) {
        std::cout << e.what() << std::endl;
        std::exit(1);
    }

    return args;
}

int main(int argc, char** argv)
{
    Arguments args = parse_args(argc, argv);

    Logger::get_instance()->add_sink(std::make_shared<ConsoleSink>(Loglevel::DEBUG));
    Logger::get_instance()->add_sink(std::make_shared<SyslogSink>(Loglevel::INFO));

    try {
        chat::ChatServer server(args.iface, args.port);
        server.start();
    }
    catch (std::runtime_error& e) {
        Logger::get_instance()->error(e.what());
    }

    return 0;
}
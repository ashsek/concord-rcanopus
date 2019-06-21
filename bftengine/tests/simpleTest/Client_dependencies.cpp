Client dependencies,

#include <cassert>
#include <thread>
#include <iostream>
#include <limits>

// bftEngine includes
#include "CommFactory.hpp"
// For commfactory,

#include "CommDefs.hpp"
// in common defs.hpp
#ifndef BYZ_COMMDEFS_HPP
#define BYZ_COMMDEFS_HPP
#if defined(_WIN32)
#include <WinSock2.h>
#include <ws2def.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include <string>
#include <stdint.h>
#include <memory>
#include <unordered_map>
#include "ICommunication.hpp"
#include <stdint.h>


#include "StatusInfo.h"
#include <functional>
#include <string>


#include "Logging.hpp"
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <stdarg.h>
#ifdef USE_LOG4CPP
#include <log4cplus/loggingmacros.h>
typedef log4cplus::Logger LoggerImpl;
#endif

#include "SimpleClient.hpp"
#include  <cstddef>
#include <stdint.h>
#include <string>
#include <set>
#include "ICommunication.hpp"

// simpleTest includes
#include "commonDefs.h"
#include "test_comm_config.hpp"
#include "test_parameters.hpp"
#include "Logging.hpp"

#ifdef USE_LOG4CPP
#include <log4cplus/configurator.h>
#endif
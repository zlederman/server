#include <string.h>
#include <string>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "logger.hh"

using namespace std;

Logger::Logger(string name) {
	_requestCount = 0;
	_name = name
}	

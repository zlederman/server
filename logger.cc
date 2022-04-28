#include <string.h>
#include <string>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <sstream>
#include "logger.hh"

#define eps 2.225e-308

using namespace std;

Logger::Logger(string name) {
	_requestCount = 0;
	_name = name;
	_maxTime = 0.0;
	_minTime = 1.79769e+308;
}	


void Logger::addRequest() {
	_requestCount += 1;
	
}
void Logger::addTime(double cpuTime) {
	if(_minTime - cpuTime > eps){
		_minTime = cpuTime;
	}
	if(_maxTime - cpuTime <  eps){
		_maxTime = cpuTime;
	}
}

string Logger::assembleHTML(){
	stringstream html;
	html << "<!DOCTYPE html>";
	html << "<html>" << "<head>";
	html << "<h1>" << _name << "</h1>" << "</head>";
	html << "<body>";
	html << "<ul>";
	html << "<li>Total Request Count: " << _requestCount << "</li>";
	html << "</ul>" << "</body>" << "</html>";
	return html.str();
}
	

void Logger::serveAsset(HTTPResponse* httpRes){
	string html;
	html = assembleHTML();
	httpRes->_bodySize = html.length() + 1;
	httpRes->_body = new char[httpRes->_bodySize];
	strcpy(httpRes->_body,html.c_str());

}


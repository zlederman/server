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
	_minURL = string("");
	_maxURL = string("");
}	


void Logger::addRequest() {
	_requestCount += 1;
	
}
void Logger::addTime(double cpuTime, string lastURL) {
	if(_minTime - cpuTime > eps){
		_minTime = cpuTime;
		_minURL = lastURL;
	}
	if(_maxTime - cpuTime <  eps){
		_maxTime = cpuTime;
		_maxURL = lastURL;
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
	html << "<li>Shortest Time: " << to_string(_minTime) << "</li>";
	html << "<li>Largest Time: " << to_string(_maxTime) << "</li>";
	html << "</ul>" << "</body>" << "</html>";
	return html.str();
}
	

void Logger::serveAsset(HTTPResponse* httpRes){
	string html;
	html = assembleHTML();
	httpRes->_bodySize = html.length() ;
	httpRes->_body = new char[httpRes->_bodySize + 1];
	strcpy(httpRes->_body,html.c_str());

}


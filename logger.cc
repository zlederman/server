#include <string.h>
#include <string>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <sstream>
#include "logger.hh"

using namespace std;

Logger::Logger(string name) {
	_requestCount = 0;
	_name = name;
}	


void Logger::addRequest() {
	_requestCount += 1;
	
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


#include <string>
#include <stdio.h>
#include <regex>
#include <vector>
#include <iostream>
#include "myhttp.hh"
using namespace std;


HTTPRequest::HTTPRequest(requestType request, string asset, vector<string> headers){
			_request = request;
			_asset = asset;
			_headers = headers;	
};
HTTPRequest* HTTPMessageFactory::parseMessage(string raw){
	string delimeter = std::string("\015\012");


}


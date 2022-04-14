#include <string>
#include <stdio.h>
#include <regex>
#include <vector>
#include <iostream>
#include "myhttp.hh"

#define get string("GET")
#define post string("POST")


using namespace std;
vector<string> splitRaw(string raw);
requestType getType(string requestHead);

HTTPRequest::HTTPRequest(requestType request, string asset, vector<string> headers){
			_request = request;
			_asset = asset;
			_headers = headers;	
};
HTTPRequest* HTTPMessageFactory::parseMessage(string raw){
	vector<string> lines;
	requestType rtype;
	lines = splitRaw(raw);
	rtype = getType(lines[0]);
	cout << "HELLO" << endl;
}

requestType getType(string requestHead){
	if(requestHead.find(get) != string::npos){
		return GET;
	}
	if(requestHead.find(post) != string::npos){
		return POST;
	}
	return ERR;
}
vector<string> splitRaw(string raw){
	
	vector<string> tokens;
	string delimeter = std::string("\015\012");
	size_t pos = 0;
	string token;
	while((pos = raw.find(delimeter)) != string::npos){
		token = raw.substr(0,pos);
		raw.erase(0,pos + delimeter.length());
		tokens.push_back(token);
	}	
	return tokens;
}

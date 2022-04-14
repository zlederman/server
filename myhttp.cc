#include <string>
#include <stdio.h>
#include <regex>
#include <vector>
#include <iostream>
#include "myhttp.hh"
using namespace std;

vector<string> splitRaw(string raw);
HTTPRequest::HTTPRequest(requestType request, string asset, vector<string> headers){
			_request = request;
			_asset = asset;
			_headers = headers;	
};
HTTPRequest* HTTPMessageFactory::parseMessage(string raw){
	vector<string> lines;
	lines = splitRaw(raw):
	cout << "HEllo"<< endl;
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

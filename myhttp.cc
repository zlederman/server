#include <string>
#include <stdio.h>
#include <regex>
#include <vector>
#include <iostream>
#include "myhttp.hh"

#define get string("GET")
#define post string("POST")
#define errString string("\0");

using namespace std;

vector<string> splitRaw(string raw);
requestType getType(string requestHead);
string getAsset(string requestHead);

HTTPRequest::HTTPRequest(requestType request, string asset, vector<string> headers){
			_request = request;
			_asset = asset;
			_headers = headers;	
};
HTTPRequest* HTTPMessageFactory::parseMessage(string raw){
	vector<string> lines;
	requestType rtype;
	string asset;
	
	lines = splitRaw(raw);
	rtype = getType(lines[0]);
	asset = getAsset(lines[0]);
	cout << "HELLO" << endl;	
}

string getAsset(string requestHead){
	string assetRoot = string("/");
	size_t pos;
	string res;
	if((pos = requestHead.find(assetRoot)) != string::npos){
		if(requestHead.at(pos - 1) != ' '){
			return errString;
		}
	}
	else{
		return errString;
	}
	while(requestHead.at(pos) != ' ' && requestHead.at(pos) != '\0'){
		res += requestHead.at(pos);
		pos++;
	}
	return res;
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

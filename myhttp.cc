#include <string>
#include <stdio.h>
#include <regex>
#include <vector>
#include <iostream>


using namespace std;


HTTPRequest::HTTPRequest(requestType request, string asset, vector<string> headers){
			_request = request;
			_asset = asset;
			_headers = headers;	
};	
static HTTPRequest* HTTPMessageFactory::parseMessage(string raw){
			cout << raw << endl;
			return NULL;
}


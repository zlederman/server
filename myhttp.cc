#include <string>
#include <stdio.h>
#include <regex>
#include <vector>
#include <iostream>
#include <myhttp.hh>
using namespace std;


HTTPRequest::HTTPRequest(requestType request, string asset, vector<string> headers){
			_request = request;
			_asset = asset;
			_headers = headers;	
};	
static HTTPRequest* HTTPMessageFactoryparseMessage(string raw){
			std::cout << raw << std::endl;
			return NULL;
}


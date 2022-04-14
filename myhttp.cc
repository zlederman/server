#include <string>
#include <stdio.h>
#include <regex>
#include <vector>
#include <iostream>


using namespace std;



static HTTPRequest::HTTPRequest* HTTPMessageFactoryparseMessage(string raw){
			cout << raw << endl;
			return NULL;
}

HTTPRequest::HTTPRequest(requestType request, string asset, vector<string> headers){
			_request = request;
			_asset = asset;
			_headers = headers;	
};	

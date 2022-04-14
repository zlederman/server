#include <string>
#include <stdio.h>
#include <regex>
#include <vector>
#include <iostream>
#include <myhttp.h>
using namespace std;
class HTTPRequest {
	public:
		requestType _request;
		string _asset;
		vector<string> _headers;
		
		HTTPRequest(requestType request, string asset, vector<string> headers){
			_request = request;
			_asset = asset;
			_headers = headers;	
		};	


};



class HTTPMessageFactory {
	public: 
		static HTTPRequest* parseMessage(string raw){
			std::cout << raw << std::endl;
			return NULL;

		}

};


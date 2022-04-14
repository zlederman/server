#include <string>
#include <stdio.h>
#include <regex>
#include <vector>

using namespace std;
enum requestType { GET, POST };
enum responseCode {a };

class HTTPMessageFactory {
	public: 
		static HTTPRequest parseMessage(string raw){
			cout<< raw << endl;
			return NULL;

		}

};

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



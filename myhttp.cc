#include <string>
#include <stdio.h>
#include <regex>
#include <vector>
#include <iostream>
#include <map>
#include "myhttp.hh"

#define get string("GET")
#define post string("POST")
#define errString string("\0");
#define rootDir string("http-root-dir")

using namespace std;

//HELPERS
vector<string> splitRaw(string raw, string delimeter);
requestType getType(string requestHead);
string getAsset(string requestHead);


// CONSTANTS LIKE HEADERS
const string HTTPMessageFactory::version = "HTTP/1.0";
const string HTTPMessageFactory::authHeader = string("WWW-Authenticate: Basic realm=\"ANTON\"");
const string HTTPMessageFactory::contentTypeHTML = string("Content-type: text/html");
const string HTTPMessageFactory::contentTypePNG = string("Content-type: image/png");
const string HTTPMessageFactory::contentTypeGIF = string("Content-type: image/gif");
const string HTTPMessageFactory::contentTypeICO = string("Content-type: image/vnd.microsoft.icon");
const string HTTPMessageFactory::contentTypeSVG = string("Content-type: image/svg+xml");
const string HTTPMessageFactory::contentLength = string("Content-Length: ");
const int HTTPMessageFactory::maxResponseHeaderSize = 2048;
const map<int,string> HTTPMessageFactory::statuses = {
	{200, string("200 OK")},
	{401, string("401 Unauthorized")},
	{400, string("400 Bad Request")},
	{404, string("404 Not Found")}
};

/*
 *
 * HTTP RESPONSE IMPL
 *
 */

HTTPResponse::HTTPResponse(int statusCode) {
	//GETS RESPONSE TYPE
	_status = HTTPMessageFactory::statuses.at(statusCode);
	if(statusCode == 401){
		//adds headers for unauthorized
		insertHeader(HTTPMessageFactory::authHeader);
	}
	_bodySize = 0;
}

HTTPResponse::~HTTPResponse(){
	//only delete body if ther is one
	if(_bodySize > 0){
		delete _body;
	}

}
void HTTPResponse::insertHeader(string header){
	_headers.push_back(header);
}

int HTTPResponse::loadRaw(char* raw){
	//builds header into one string 
	//laods body and header into one byte arr
	string response;
	int responseLen;
	response += HTTPMessageFactory::version;
	response += " ";
	response += _status;
	response += " \r\n";

	for(string& header: _headers) {
		response += header;
		response += "\r\n";
	}
	response += "\r\n";
	//copy header string into raw bytes and then  copy body (can be 0 bytes)
	memcpy(raw,response.c_str(),response.length());	
	memcpy(raw + response.length(), _body, _bodySize);
	//return size for use later
	return response.length() + _bodySize;

}

/*
 *
 * HTTP REQUEST IMPL
 * 
 */
HTTPRequest::HTTPRequest(requestType request, string asset,vector<string> queryParams, vector<string> headers){
			_request = request;
			_asset = asset;
			_queryParams = queryParams;
			_headers = headers;	
}

HTTPRequest::~HTTPRequest(){}


string HTTPRequest::toString(){
	//for logging use
	string res;
	string rawAsset;
	switch(_request){
		case GET:
			res += get;
			break;
		case POST:
			res += post;
			break;
		default:
			res += "ERROR INVALID REQ";
			return res;
	}
	rawAsset = _asset;
	rawAsset.erase(0,rootDir.length());
	res += " ";
 	res += rawAsset;	
	return res;
}

string HTTPRequest::findHeader(string headerName) {
	for(string& header : _headers){
		if(header.find(headerName) != string::npos){
			return header;
		}
	}
	return errString;
}

/*
 *
 * FACTORY METHODS
 *
 */

HTTPRequest* HTTPMessageFactory::parseMessage(string raw){
	vector<string> lines;
	requestType rtype;
	string asset;
	string rawAsset;
	vector<string> headers;
	vector<string> queryParams;
	int idxQuery;

	lines = splitRaw(raw,string("\015\012"));
	rtype = getType(lines[0]);
	
	//format asset
	rawAsset = getAsset(lines[0]);
	idxQuery = rawAsset.find("?");
	if(idxQuery != string::npos){
		queryParams = splitRaw(rawAsset.substr(idxQuery+1,rawAsset.length()),string(";"));
		rawAsset = rawAsset.substr(0,idxQuery);
	}
	if(rawAsset == string("/")){
		rawAsset = string("/index.html");
	}

	asset = string("http-root-dir");
	asset += rawAsset;
	lines.pop_back();

	for(size_t i = 1; i < lines.size(); i++){
		headers.push_back(lines.at(i));
	}
	return new HTTPRequest(rtype,asset,queryParams,headers);	
}

HTTPResponse* HTTPMessageFactory::initResponse(int statusCode){
	return new HTTPResponse(statusCode);
}


/*
 * HELPERS 
 */

string getAsset(string requestHead){
	string assetRoot = string("/");
	size_t pos;
	string res; 
	if((pos = requestHead.find(assetRoot)) != string::npos){
		if(requestHead.at(pos - 1) != ' '){
			return errString;
		}//fails if this matches HTTP/1.0 => no asset
	}
	else{
		return errString;
	}
	while(requestHead.at(pos) != ' ' && requestHead.at(pos) != '\0'){
		res += requestHead.at(pos);
		pos++;
	}//gets the asset string
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
vector<string> splitRaw(string raw, string delimeter){
	//splits into a vector of chunks by carriage return
	vector<string> tokens;
	size_t pos = 0;
	string token;
	while((pos = raw.find(delimeter)) != string::npos){
		token = raw.substr(0,pos);
		raw.erase(0,pos + delimeter.length());
		tokens.push_back(token);
	}
	if(delimeter != string("\015\012")){
		tokens.push_back(raw);
	}
	return tokens;
}


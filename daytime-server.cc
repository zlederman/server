
const char * usage =
"                                                               \n"
"daytime-server:                                                \n"
"                                                               \n"
"Simple server program that shows how to use socket calls       \n"
"in the server side.                                            \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   daytime-server <port>                                       \n"
"                                                               \n"
"Where 1024 < port < 65536.             \n"
"                                                               \n"
"In another window type:                                       \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where daytime-server  \n"
"is running. <port> is the port number you used when you run   \n"
"daytime-server.                                               \n"
"                                                               \n"
"Then type your name and return. You will get a greeting and   \n"
"the time of the day.                                          \n"
"                                                               \n";


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/wait.h>
#include "myhttp.hh"


#define errString string("\0");
#define rootDir "http-root-dir/";
#define index "index.html";

using namespace std;

int QueueLength = 5;
HTTPMessageFactory* httpFactory = new HTTPMessageFactory();

string PASS = ": Basic cGFzc3dvcmQ6dXNlcm5hbWU=";
// Processes time request
void processClient( int socket );
HTTPRequest* buildHTTPRequest(int fd);
bool authenticate(HTTPRequest* httpReq);
void log(string status);
int initIncoming(int masterSocket);
void forkServer(int masterSocket);
void log(string status){
	cout << "\033[1;32m[ INFO ]\033[0m " << status << endl; 
}

int
main( int argc, char ** argv )
{
	

				// Print usage if not enough arguments
  if ( argc < 2 ) {
    fprintf( stderr, "%s", usage );
    exit( -1 );
  }

  // Get the port from the arguments
  int port = atoi( argv[1] );
  
  // Set the IP address and port for this server
  struct sockaddr_in serverIPAddress; 
  memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_addr.s_addr = INADDR_ANY;
  serverIPAddress.sin_port = htons((u_short) port);
  
  // Allocate a socket
  int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
  if ( masterSocket < 0) {
    perror("socket");
    exit( -1 );
  }

  // Set socket options to reuse port. Otherwise we will
  // have to wait about 2 minutes before reusing the sae port number
  int optval = 1; 
  int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
		       (char *) &optval, sizeof( int ) );
   
  // Bind the socket to the IP address and port
  int error = bind( masterSocket,
		    (struct sockaddr *)&serverIPAddress,
		    sizeof(serverIPAddress) );
  if ( error ) {
    perror("bind");
    exit( -1 );
  }
  
  // Put socket in listening mode and set the 
  // size of the queue of unprocessed connections
  error = listen( masterSocket, QueueLength);
  if ( error ) {
    perror("listen");
    exit( -1 );
  }

	forkServer(masterSocket);	 
  
}


void iterativeServer(int serverSocket) {
	int clientSocket;
	while(1) {
		clientSocket = initIncoming(serverSocket);
		processClient(clientSocket);
		close(clientSocket);
	}
}
void forkServer(int serverSocket) {
	int clientSocket;
	int ret;
	while(1) {
		clientSocket = initIncoming(serverSocket);
		ret = fork();

		if(ret == 0){
			processClient(clientSocket);
			exit(0);
		}
		if(ret < 0){
			perror("fork");
			exit(-1);
		}
		close(clientSocket);
		waitpid(ret,0,0);
	}
}

int initIncoming(int masterSocket) {
	struct sockaddr_in clientIPAddress;
	int alen = sizeof(clientIPAddress);
	int slaveSocket = accept( masterSocket,
									(struct sockaddr *)&clientIPAddress,
									(socklen_t*)&alen);
	log(to_string(clientIPAddress.sin_addr));
	if(slaveSocket < 0){
		perror("accept");
		exit(-1);
	}
	return slaveSocket;
}

bool authenticate(HTTPRequest* httpReq){	
	string pass;
	string authHeader = string("Authorization");
	string header = httpReq->findHeader(authHeader);
	string delim = string(": Basic ");

	size_t idx = header.find(delim);
	if(header == string("\0") || idx == string::npos){
		return false;
	}	
	pass = header.substr(idx ,header.length() - idx);
	if(pass != PASS){
		return false;
	}
	return true;
}

string readRaw(int slaveFd){
	int n;
	string raw;
	unsigned char newChar;	
	unsigned char oldChar;


	while (( n = read( slaveFd, &newChar, sizeof(newChar) ) ) > 0 ) {
		if(oldChar == '\012' && newChar == '\015'){
			
			raw += newChar;
			//catches double carriage return
			if((n = read(slaveFd,&newChar, sizeof(newChar))) > 0) {
				if(newChar == '\012'){
					raw += newChar;
					break;		
				}
			}

		}
    raw += newChar;
		oldChar = newChar;
  }

	return raw;
}

HTTPResponse* initGetResponse(HTTPRequest* request){
	int responseCode = 400;
	//check if authed
	if(!authenticate(request)){
		responseCode = 401;
		return httpFactory->initResponse(responseCode);
	}
	//check if directory is valid
	if(request->_asset == string("/")){
		responseCode = 200;
	}
	else{
		responseCode = 404;
	}

	return httpFactory->initResponse(responseCode);
}

string getData(string asset){
	string data;
	string name;
	FILE* f;
	name = rootDir;
	if(asset == "/"){
		name += index;
	}
	else{
		name += asset;
	}
	f = fopen(name.c_str(),"r");
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);

	char where[size];
	rewind(f);
	fread(where,sizeof(char),size,f);
	return string(where);

}
void processClient(int fd){

	HTTPRequest* httpReq;
  HTTPResponse* httpRes;
	string raw_response;
				//get http request object
	httpReq = buildHTTPRequest(fd);
	switch(httpReq->_request){
		case GET:
			httpRes = initGetResponse(httpReq);
			break;
		case POST:
			break;
		default:
			//handle unknown request type
			break;
	}
	if(httpRes->_status == string("401 Unauthorized")){
		httpRes->_headers.push_back(HTTPMessageFactory::authHeader);
	}
	if(httpRes->_status == string("200 OK")){
		httpRes->_headers.push_back(HTTPMessageFactory::contentTypeHTML);
		httpRes->_body = getData(httpReq->_asset);
	}
	log(httpRes->_status);
	raw_response = httpRes->toString();
	write(fd,raw_response.c_str(),raw_response.length());
	//interpret http object
	// build response
	// send response

}

HTTPRequest*
buildHTTPRequest( int fd )
{
  HTTPRequest* req;
	string raw_req;
  
	raw_req = readRaw(fd);  
  
	req = httpFactory->parseMessage(raw_req);
	log(req->toString());
	return req;  
}

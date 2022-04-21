
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
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include "myhttp.hh"


#define errString string("\0");
#define rootDir "http-root-dir/";
#define index "/index.html";

using namespace std;

int QueueLength = 5;
HTTPMessageFactory* httpFactory = new HTTPMessageFactory();

string PASS = ": Basic cGFzc3dvcmQ6dXNlcm5hbWU=";
// Processes time request
void processClient( int socket );
HTTPRequest* buildHTTPRequest(int fd);
bool authenticate(HTTPRequest* httpReq);
void log(string status);
bool validate(string path);
int initIncoming(int masterSocket);
string getIP(struct in_addr ip_struct);
void iterativeServer(int masterSocket);
void forkServer(int masterSocket);
char* dispatchOK(HTTPResponse* httpRes, HTTPRequest* httpReq, int* rawLength);
void log(string status){
	cout << "\033[1;32m[ INFO ]\033[0m " << status << endl; 
}


extern "C" void reap(int sig){
	int status;
	string logStr;
	int res = waitpid(-1,&status,WNOHANG);
	if(res >0){
		logStr += to_string(res);
		logStr += " exited.";
	}

}

extern "C" void quit(int sig){
	waitpid(-1,NULL,WNOHANG);
	exit(0);
}


int
main( int argc, char ** argv )
{
	

				// Print usage if not enough arguments
  if ( argc < 2 ) {
    fprintf( stderr, "%s", usage );
    exit( -1 );
  }
	//registering sig int handler
	//used for quiting server
	struct sigaction sa_ctrlc;
	sa_ctrlc.sa_handler = quit;
	sigemptyset(&sa_ctrlc.sa_mask);
	sa_ctrlc.sa_flags = SA_RESTART;
	if(sigaction(SIGINT,&sa_ctrlc,NULL)){
		perror("sigaction");
		exit(2);
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
	iterativeServer(masterSocket);
	//forkServer(masterSocket);	 
  
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
		while(waitpid(0,0,0)>0);
	}
}

string getIP(struct in_addr ip_struct){
	string res;
	uint32_t ip_num = ip_struct.s_addr;
	res += "Client IP ";
	res += to_string(ip_num & (0xFF));
	res += ".";	
	res += to_string((ip_num & (0xFF << 8)) >> 8);
	res += ".";
	res += to_string((ip_num & (0xFF << 16)) >> 16);
	res += ".";
	res += to_string((ip_num & (0xFF << 24)) >> 24);
	return res;
}

int initIncoming(int masterSocket) {
	struct sockaddr_in clientIPAddress;
	int alen = sizeof(clientIPAddress);
	int slaveSocket = accept( masterSocket,
									(struct sockaddr *)&clientIPAddress,
									(socklen_t*)&alen);
	log(getIP(clientIPAddress.sin_addr));
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

string getDirectory(string path){
	for(int i = path.length() - 1; i >= 0; i--){
		if(path.at(i) == '/'){
			return path.substr(0,i+1);
		}
	}
	return string("/");
}
bool validate(string path){
	DIR* dir;
	string directory;
	if(path.find("..") != string::npos){
		return false;
	}
	path.erase(path.begin());
	directory += rootDir;
	directory += getDirectory(path);
	// check if path goes into parent

	// check if directory or file exists
	dir = opendir(directory.c_str());
	if(dir) {
		closedir(dir);
		return true;
	}
	else{
		return false;
	}
}


HTTPResponse* initGetResponse(HTTPRequest* request){
	int responseCode = 400;
	//check if authed
	if(!authenticate(request)){
		responseCode = 401;
		return httpFactory->initResponse(responseCode);
	}
	//check if directory is valid
	if(validate(request->_asset)){
		responseCode = 200;
	}
	else{
		responseCode = 404;
	}

	return httpFactory->initResponse(responseCode);
}

void getBody(string asset, HTTPResponse* httpRes){
	string name;
	FILE* f;
	char* where;
	name = rootDir;
	
	if(asset == "/"){
		name += index;
	}
	else{
		name += asset;
	}
	f = fopen(name.c_str(),"r");
	fseek(f, 0, SEEK_END);
	httpRes->_bodySize = ftell(f);
	httpRes->_body = new char[httpRes->_bodySize];
	rewind(f);
	fread(httpRes->_body,sizeof(char),httpRes->_bodySize,f);
}



string getMIMEType(string asset){
	string png = string(".png");
	string ico = string(".ico");
	string gif = string(".gif");
	string html = string(".html");
	string svg = string(".svg");

	if(asset.find(png) != string::npos) { 
		return HTTPMessageFactory::contentTypePNG;
 	}
	if(asset.find(ico) != string::npos) { 
		return HTTPMessageFactory::contentTypeICO;
	}
	if(asset.find(gif) != string::npos) {
		return HTTPMessageFactory::contentTypeGIF;
	}
	if(asset.find(html) != string::npos){
		return HTTPMessageFactory::contentTypeHTML;
	}
	if(asset.find(svg) != string::npos){
		return HTTPMessageFactory::contentTypeSVG;
	}
	return errString;
}

char* dispatchOK(HTTPResponse* httpRes, HTTPRequest* httpReq, int* rawLength){
	string contentTypeHeader;
	string contentLengthHeader;
	char* raw;

	contentTypeHeader = getMIMEType(httpReq->_asset);
	contentLengthHeader = HTTPMessageFactory::contentLength;
	getBody(httpReq->_asset,httpRes);
	contentLengthHeader += to_string(httpRes->_bodySize);

	
	raw = (char*) malloc(sizeof(char)* (HTTPMessageFactory::maxResponseHeaderSize + httpRes->_bodySize));
	*rawLength = httpRes->loadRaw(raw);
	return raw;

	
}
void processClient(int fd){
	char* raw;
	int* rawLength;
	HTTPRequest* httpReq;
  HTTPResponse* httpRes;
	
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

	rawLength = (int*) malloc(sizeof(int));
	if(httpRes->_status == string("200 OK")){	
		raw = dispatchOK(httpRes,httpReq,rawLength);
	}
	else{
		raw = (char*) malloc(sizeof(char*) * HTTPMessageFactory::maxResponseHeaderSize);
		*rawLength = httpRes->loadRaw(raw);	
	}
	log(httpRes->_status);
	write(fd,raw,*rawLength);
	
	//delete httpReq;
	//delete httpRes;
	free(rawLength);
	free(raw);

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

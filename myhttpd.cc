#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
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
#include <regex>
#include <errno.h>
#include "myhttp.hh"
#include "dirbrowser.hh"
#include "logger.hh"

#define OK string("200 OK")
#define errString string("/0")
#define IS_CGI true
using namespace std;

int QueueLength = 5;
pthread_mutex_t lock;

HTTPMessageFactory* httpFactory = new HTTPMessageFactory(); //initialize factory class
DirBrowser* dirBrowser = new DirBrowser();
Logger* logger = new Logger(string("Zachary Lederman"),time(NULL));

string PASS = ": Basic cGFzc3dvcmQ6dXNlcm5hbWU="; //definitely not the smart thing to do
extern "C" void quit(int sig);
extern "C" void reap(int sig);

string processClient( int socket );
HTTPRequest* buildHTTPRequest(int fd);
string getIP(struct in_addr ip_struct);
bool authenticate(HTTPRequest* httpReq);
void handleCGI(int clientFd,HTTPRequest* httpReq);
void log(string status);
void delegateRequest(int fd,HTTPResponse* httpRes, HTTPRequest* httpReq);
HTTPResponse* initGetResponse(HTTPRequest* request);
bool validate(string path);
char* dispatchOK(HTTPResponse* httpRes, HTTPRequest* httpReq, int* rawLength);
char* dispatchStat(HTTPResponse* httpRes, HTTPRequest* httpReq, int* rawLength);
int initIncoming(int masterSocket);
int initIncoming_r(int masterSocket);

void iterativeServer(int masterSocket);
void forkServer(int masterSocket);
void poolThreadServer(int serverSocket);
void lazyThreadServer(int serverSocket);


extern "C" void * processClientWrapper(void * data);
extern "C" void* iterativeServer_r(void* data);

int
main( int argc, char ** argv )
{
	
  int port;
	string serverType;
	if(argc == 3){
		port = atoi(argv[2]);
		serverType = string(argv[1]);
	}
 	if ( argc < 2 ) {
			port = 8888;
			serverType = string("-d");
	}

	else if(argc < 3){
		if(argv[1][0] == '-'){
			port = 8888;
			serverType = string(argv[1]);
		}
		else{
			port = atoi(argv[1]);
			serverType = string("-d");
		}
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

	struct sigaction sa_zombie;
	sa_zombie.sa_handler = reap;
	sigemptyset(&sa_zombie.sa_mask);
	sa_zombie.sa_flags = SA_RESTART;
	if(sigaction(SIGCHLD, &sa_zombie, NULL)){
		perror("sigaction");
		exit(2);
	}
	
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
	if(serverType == string("-p")){
		log(string("starting Pool Of 5 Threads "));
		poolThreadServer(masterSocket);
	}
	else if(serverType == string("-t")){  	
		log(string("starting lazy threads"));
		lazyThreadServer(masterSocket);
	}
	else if(serverType == string("-f")){		
		log(string("starting lazy fork"));
		forkServer(masterSocket);	 
	}
	else{
		log(string("starting iterative"));
		iterativeServer(masterSocket);		
	}
}


/*
 * LOGGING FUNCTIONS
 */

void log(string status){
	cout << "\033[1;32m[ INFO ]\033[0m " << status << endl; 
}

string getIP(struct in_addr ip_struct){
	//obtains 8 bit chunks of ip address
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

/*
 * PROCESS COLLECTORS
 */

extern "C" void reap(int sig){
	int status;
	string logStr;
	int res = waitpid(-1,&status,WNOHANG);
	if(res >0){
		logStr += to_string(res);
		logStr += " exited.";
		log(logStr);
	}
		

}

extern "C" void quit(int sig){
	waitpid(-1,NULL,WNOHANG);
	exit(0);
}
 

/*
 * SERVER TYPE DEFINITIONS
 */

void iterativeServer(int serverSocket) {
	int clientSocket;
	clock_t start,end;
	double cpuTime;
	string lastURL;
	string* ipaddr;
	while(1) {
		clientSocket = initIncoming(serverSocket);
		start = clock();
		lastURL = processClient(clientSocket);//returns req url
		close(clientSocket);
		end = clock();
		cpuTime = ((double) (end - start)) / CLOCKS_PER_SEC; //calc time for request
		logger->addTime(cpuTime,lastURL);
	}

}

void poolThreadServer(int serverSocket){
	pthread_t threads[4];
	//initializes mutex
	if(pthread_mutex_init(&lock,NULL)!= 0){
		perror("mutex");
		exit(-1);
	}
	for(int i = 0; i < 4; i++){
		pthread_create(&threads[i],NULL,iterativeServer_r,(void*) serverSocket);	
	}
	iterativeServer_r((void*) serverSocket);
}

void lazyThreadServer(int serverSocket){
	
	while(1){	
		pthread_t thread;
		int clientSocket = initIncoming(serverSocket);
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
		pthread_create(&thread,&attr,processClientWrapper,(void*) clientSocket);	
	}
}

void forkServer(int serverSocket) {
	int clientSocket;
	int ret;
	clock_t start;
	int  end;
	double cpuTime;
	while(1) {
		
		
		clientSocket = initIncoming(serverSocket);
		start = clock();
		ret = fork();

		if(ret == 0){
			processClient(clientSocket);
			exit(clock());
		}
		if(ret < 0){
			perror("fork");
			exit(-1);
		}
		close(clientSocket);
		//collect processes
		waitpid(-1,&end,WNOHANG);
		cpuTime = ((double) (end - start)) / CLOCKS_PER_SEC;
		logger->addTime(cpuTime,string(""));
		logger->addRequest();
	}
}

/*
 * MULTITHREADING ENTRY POINTS
 */

extern "C" 
void* iterativeServer_r(void* data){
	//re-entrant version of iterative server
	//jsut contains lock for accept
	int serverSocket = (size_t) data;
	int clientSocket;
	clock_t start,end;
	double cpuTime;
	string lastURL;
	while(1){
			clientSocket = initIncoming(serverSocket);
			start = clock();
			lastURL = processClient(clientSocket);
			close(clientSocket);
			end = clock();
			cpuTime = ((double) (end - start)) / CLOCKS_PER_SEC;
			logger->addTime(cpuTime,lastURL);
	}
}
extern "C"
void * processClientWrapper(void * data){
	//wraps the process client function for threads
	clock_t start, end;
	string lastURL;
	double cpuTime;

	start = clock();
	int clientSocket = (size_t) data;
	lastURL = processClient(clientSocket);
	close(clientSocket);
	end = clock();
	cpuTime = ((double) (end - start)) / CLOCKS_PER_SEC;
	logger->addTime(cpuTime,lastURL);
	pthread_exit(NULL);
	return NULL;
}

/*
 * CLIENT SOCKET ACCEPTORS
 */

int initIncoming_r(int masterSocket){
	//re-entrant version of initIncoming
	struct sockaddr_in clientIPAddress;
	int alen = sizeof(clientIPAddress);

	pthread_mutex_lock(&lock);
	int slaveSocket = accept( masterSocket,
									(struct sockaddr *)&clientIPAddress,
									(socklen_t*)&alen);
	pthread_mutex_unlock(&lock);
	if(slaveSocket < 0){
		perror("accept");
		exit(-1);
	}
	return slaveSocket;

	
}
int initIncoming(int masterSocket) {
	struct sockaddr_in clientIPAddress;
	int alen = sizeof(clientIPAddress);
	int slaveSocket = accept( masterSocket,
									(struct sockaddr *)&clientIPAddress,
									(socklen_t*)&alen);
	if(slaveSocket < 0){
		perror("accept");
		exit(-1);
	}
	return slaveSocket;
}

/*
 * TOP LEVEL REQ/RES FUNCTION
 */

string processClient(int fd){
	string lastURL;
	char* raw;
	int* rawLength;
	HTTPRequest* httpReq;
  HTTPResponse* httpRes;
	string cgi = string("cgi-bin");
	httpReq = buildHTTPRequest(fd); //reads and returns constructed request
	logger->addRequest();
	switch(httpReq->_request){
		case GET:
			httpRes = initGetResponse(httpReq); //creates response from validation steps
			break;
		case POST:
			break;
		default:
			//handle unknown request type
			break;
	}

	delegateRequest(fd,httpRes,httpReq);
	lastURL = httpReq->toString();
	logger->logRequest(lastURL);
	delete httpReq;
	delete httpRes;
	return lastURL;

}

void delegateRequest(int fd,HTTPResponse* httpRes, HTTPRequest* httpReq){
	int * rawLength;
	char* raw;
	regex statPattern = regex("stats.*");
	regex cgiPattern ("cgi-bin/.+");
	smatch cgiMatch;
	smatch statMatch;
	rawLength = (int*) malloc(sizeof(int));
	
	if(regex_search(httpReq->_asset,cgiMatch,cgiPattern)){
		raw = (char*) malloc(sizeof(char*) * HTTPMessageFactory::maxResponseHeaderSize);
		*rawLength = httpRes->loadRaw(raw,IS_CGI);
		write(fd,raw,*rawLength);
		handleCGI(fd,httpReq);
	}
	else if(regex_search(httpReq->_asset,statMatch,statPattern)){
		raw = dispatchStat(httpRes,httpReq,rawLength);
		write(fd,raw,*rawLength);
	}
	else if(httpRes->_status == OK){
		raw = dispatchOK(httpRes,httpReq, rawLength);
		write(fd,raw,*rawLength);
	}
	else{
		raw = (char*) malloc(sizeof(char*) * HTTPMessageFactory::maxResponseHeaderSize);
		*rawLength = httpRes->loadRaw(raw,!IS_CGI);
		write(fd,raw,*rawLength);
	}
	log(httpRes->_status);
	free(raw);
	free(rawLength);


}

/*
 * INPUT VALIDATION
 */ 

bool authenticate(HTTPRequest* httpReq){	
	string pass;
	string authHeader = string("Authorization");
	string header = httpReq->findHeader(authHeader);
	string delim = string(": Basic ");

	size_t idx = header.find(delim);
	//checks if they even sent the pass
	if(header == string("\0") || idx == string::npos){
		return false;
	}	
	pass = header.substr(idx ,header.length() - idx); //extracts pass
	if(pass != PASS){
		//checks if pass is correct
		return false;
	}
	return true;
}

bool validate(string path){
	//checks if try to access parent
	if(path == string("http-root-dir/stats")){
			return true;
	}
	if(path.find("..") != string::npos){
		return false;
	}
	//checks if file exists
	if(access(path.c_str(), F_OK) == 0){
		return true;
	}
	else{
		return false;
	}
}


HTTPResponse* initGetResponse(HTTPRequest* request){
	//default response is 400 Bad Request
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

/*
 * INFO GETTERS
 */

void getBody(string	asset, HTTPResponse* httpRes){
	FILE* f;
	char* where;
	//gets size
	f = fopen(asset.c_str(),"r");
	fseek(f, 0, SEEK_END);
	httpRes->_bodySize = ftell(f);

	httpRes->_body = new char[httpRes->_bodySize];
	rewind(f);
	//reads body into response object
	fread(httpRes->_body,sizeof(char),httpRes->_bodySize,f);
}



string getMIMEType(string asset){
	//searches file type 
	// slow as ever
	string png = string(".png");
	string ico = string(".ico");
	string gif = string(".gif");
	string html = string(".html");
	string svg = string(".svg");
	string xbm = string(".xbm");
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
	if(asset.find(xbm) != string::npos){
		return HTTPMessageFactory::contentTypeXBM;
	}
	return errString;
}

/*
 * OK RESPONSE BUILDER
 */

char* dispatchOK(HTTPResponse* httpRes, HTTPRequest* httpReq, int* rawLength){
	string contentTypeHeader;
	string contentLengthHeader;
	char* raw;

	dirBrowser->serveAsset(httpReq->_asset,httpRes,httpReq->_queryParams);
	contentTypeHeader = getMIMEType(httpReq->_asset); //gets type of body
	contentLengthHeader = HTTPMessageFactory::contentLength;  
	contentLengthHeader += to_string(httpRes->_bodySize);//sends over content length
	//build response headers
	httpRes->insertHeader(contentLengthHeader);
	httpRes->insertHeader(contentTypeHeader);
		
	raw = (char*) malloc(sizeof(char)* (HTTPMessageFactory::maxResponseHeaderSize + httpRes->_bodySize)); //allocates space for body and header
	*rawLength = httpRes->loadRaw(raw,!IS_CGI);//loads header + body into byte array
	return raw;

	
}

char* dispatchStat(HTTPResponse* httpRes, HTTPRequest* httpReq, int* rawLength){
	char* raw;
	string contentLengthHeader;
	logger->serveAsset(httpRes);//logger serves the html
	httpRes->insertHeader(HTTPMessageFactory::contentTypeHTML);
	contentLengthHeader = HTTPMessageFactory::contentLength;
	contentLengthHeader += to_string(httpRes->_bodySize);
	httpRes->insertHeader(contentLengthHeader);
	raw = (char*) malloc(sizeof(char)* (HTTPMessageFactory::maxResponseHeaderSize + httpRes->_bodySize));
	*rawLength = httpRes->loadRaw(raw,!IS_CGI);
	return raw;
}

void setEnvVars(HTTPRequest* httpReq){
	string envVars;	
	setenv("REQUEST_METHOD","GET",1);
	if(httpReq->_queryParams.size() > 0){
			envVars = httpReq->_queryParams.at(0);
			setenv("QUERY_STRING",envVars.c_str(),1);
		}

}

void handleCGI(int clientFd,HTTPRequest* httpReq){
	int clientCopy;
	int pid;
	std::vector<const char*> args;
	string exeString;
	string envVars;
	string ext;  
	ext = httpReq->_asset.substr(httpReq->_asset.length() - 3, httpReq->_asset.length()); 
	clientCopy = dup(clientFd);
	if(ext == string(".so")){
		httpReq->_asset = string("use-dlopen");
	}	
	pid = fork();
	if(pid == 0){
		dup2(clientCopy,1);
		setEnvVars(httpReq);
		exeString += httpReq->_asset;
		args.push_back(exeString.c_str());
		args.push_back(NULL);
		execvp(exeString.c_str(), const_cast<char* const*>(args.data()));
	}
	if(pid < 0){
		perror("fork");
		exit(-1);
	}
	
	close(clientCopy);
}


/*
 * RAW REQUEST HANDLERS
 */ 

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
					return raw;
				}
			}

		}
    raw += newChar;
		oldChar = newChar;
  	}

	return raw;
}

HTTPRequest*
buildHTTPRequest( int fd )
{
  HTTPRequest* req;
	string raw_req;
  
	raw_req = readRaw(fd); //extract string from raw request 
  
	req = httpFactory->parseMessage(raw_req); //parse into HTTPRequest OBJ
	req->_ip = httpFactory->getIP(fd);
	log(req->toString()); 
	return req;  
}

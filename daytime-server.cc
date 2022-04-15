
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
#include "myhttp.hh"

#define errString string("\0");
using namespace std;

int QueueLength = 5;
HTTPMessageFactory* httpFactory = new HTTPMessageFactory();
string PASS;
// Processes time request
void processClient( int socket );
HTTPRequest* buildHTTPRequest(int fd);
bool authenticate(HTTPRequest* httpReq);

int
main( int argc, char ** argv )
{
	

				// Print usage if not enough arguments
  if ( argc < 2 ) {
    fprintf( stderr, "%s", usage );
    exit( -1 );
  }
  PASS = "YOLO";
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

  while ( 1 ) {

    // Accept incoming connections
    struct sockaddr_in clientIPAddress;
    int alen = sizeof( clientIPAddress );
    int slaveSocket = accept( masterSocket,
			      (struct sockaddr *)&clientIPAddress,
			      (socklen_t*)&alen);

    if ( slaveSocket < 0 ) {
      perror( "accept" );
      exit( -1 );
    }
    // Process request.
    processClient( slaveSocket );

    // Close socket
    close( slaveSocket );
  }
  
}
bool authenticate(HTTPRequest* httpReq){	
	string pass;
	string authHeader = string("Authorization");
	string header = httpReq->findHeader(authHeader);
	string delim = string(": Basic ");

	size_t idx = header.find(delim);
	if(header == errString || idx == string::npos){
		return false;
	}	
	pass = header.substr(idx,header.length() - idx);
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
    raw+= newChar;
		oldChar = newChar;
  }
	return raw;
}

HTTPResponse* initGetResponse(HTTPRequest* request){
	int responseCode = 0;
	//check if authed
	//check if directory is valid
	if(request->_asset == string("/")){
		responseCode = 200;
	}
	return new HTTPResponse(200);
}
void processClient(int fd){
	HTTPRequest* httpReq;
  HTTPResponse* httpRes;
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
	//interpret http object
	// build response
	// send response

}

HTTPRequest*
buildHTTPRequest( int fd )
{
  HTTPRequest* req;
	string delimiter = ("\012"); 
	string raw_req;
  
	raw_req = readRaw(fd);  
  req = httpFactory->parseMessage(raw_req);
	

  time_t now;
  time(&now);
  char	*timeString = ctime(&now);

  // Send name and greetings
  const char * hi = "\nHi ";
  const char * timeIs = " the time is:\n";
  write( fd, hi, strlen( hi ) );
  
  // Send the time of day 
  write(fd, timeString, strlen(timeString));

  // Send last newline
  const char * newline="\n";
  write(fd, newline, strlen(newline));
	return req;  
}

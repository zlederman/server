#include <pthread.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <sstream>
#include <semaphore.h>
#include <fcntl.h>
#include "logger.hh"

#define eps 2.225e-308

using namespace std;
#define MAX 5
BoundedBuffer::BoundedBuffer(string logFile, int numThreads){
	_head = 0;
	_numThreads = numThreads;
	_tail = 0;
	_logFile = logFile;
	//sem_init(&_emptySem,0,0);
	sem_init(&_fullSem,0,numThreads);
	pthread_mutex_init(&_mutex,NULL);
}

void BoundedBuffer::enqueue(string request){
	pthread_mutex_lock(&_mutex);
	if(_tail == MAX - 1){
		writeBuff();
		pthread_mutex_unlock(&_mutex);
		return;
	}
	_queue[_tail] = request;
	_tail = (_tail+1)% MAX;
	pthread_mutex_unlock(&_mutex);
	//sem_post(&_emptySem);
}
void BoundedBuffer::writeBuff() {
	//sem_wait(&_emptySem);
	int fd;
	pthread_mutex_lock(&_mutex);
	if(_tail != MAX - 1){
		pthread_mutex_unlock(&_mutex);
		return;
	}
	fd = open(_logFile.c_str(),O_CREAT | O_APPEND | O_RDWR,  0777);
	for(string req : _queue){
		write(fd, req.c_str(), req.length()  );
	}	
	close(fd);	
	_tail = 0;
	//for(int i = 0 ; i < _numThreads; i++){
	//	sem_post(&_fullSem);
//	}
	pthread_mutex_unlock(&_mutex);
}


Logger::Logger(string name) {
	_requestCount = 0;
	_name = name;
	_maxTime = 0.0;
	_minTime = 1.79769e+308;
	_minURL = string("");
	_maxURL = string("");
	pthread_mutex_init(&_requestLock,NULL);
	pthread_mutex_init(&_timeLock,NULL);
}	


void Logger::addRequest() {
	pthread_mutex_lock(&_requestLock);
	_requestCount += 1;
	pthread_mutex_unlock(&_requestLock);
	
}

void Logger::logRequest(string request){
	request += "\n";
	buff->enqueue(request);
}

void Logger::dump(){
	buff->writeBuff();
}
void Logger::addTime(double cpuTime, string lastURL) {
	pthread_mutex_lock(&_timeLock);
	if(_minTime - cpuTime > eps){
		_minTime = cpuTime;
		_minURL = lastURL;
	}
	if(_maxTime - cpuTime <  eps){
		_maxTime = cpuTime;
		_maxURL = lastURL;
	}
	pthread_mutex_unlock(&_timeLock);
}

string Logger::assembleHTML(){
	stringstream html;
	html << "<!DOCTYPE html>";
	html << "<html>" << "<head>";
	html << "<h1>" << _name << "</h1>" << "</head>";
	html << "<body>";
	html << "<ul>";
pthread_mutex_lock(&_requestLock);
	html << "<li>Total Request Count: " << _requestCount << "</li>";
	pthread_mutex_unlock(&_requestLock);
	pthread_mutex_lock(&_timeLock);
	html << "<li>Shortest Time: " << to_string(_minTime == 1.79769e308 ? 0 : _minTime) << "s " << _minURL <<"</li>";
	html << "<li>Largest Time: " << to_string(_maxTime) << "s " <<_maxURL <<"</li>";
	pthread_mutex_unlock(&_timeLock);
	html << "</ul>" << "</body>" << "</html>";
	return html.str();
}


void Logger::serveAsset(HTTPResponse* httpRes){
	string html;
	html = assembleHTML();
	httpRes->_bodySize = html.length() ;
	httpRes->_body = new char[httpRes->_bodySize + 1];
	strcpy(httpRes->_body,html.c_str());

}






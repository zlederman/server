#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "dirBrowser.hh"
#include "myhttp.hh"

#define _DIR 0;
#define _FILE 1;

using namespace std;
DirEntry::DirEntry(string fname, struct stat fattr){
	_name = fname;
	_size = fattr.st_size;
	_modified = fattr.st_mtime;
}


char* DirBrowser::buildHTML(string path,enum sortBy by, enum sortOrder order){
	vector<DirEntry*> entries;
	
}


void loadFile(string asset,HTTPResponse* httpRes){
	FILE* f;
	f = open(asset.c_str(),"r"):
	fseek(f,0,SEEK_END);
	httpRes->_bodySize = ftell(f);
	httpRes->_body = new char[httpRes->_bodySize];
	rewind(f);
	fread(httpRes->_body,sizeof(char),httpRes->_bodySize,f);
}

void serveAsset(string asset, HTTPResponse* httpRes){
	struct stat pathStat;
	FILE* f;

	stat(asset.c_str(), &pathStat);
	if(S_ISREG(pathStat.st_mode)){
		loadFile( asset, httpRes);
	}
	if(S_ISDIR(pathStat.st_mode)){
		/* something */
	}

}

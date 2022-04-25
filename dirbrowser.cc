#include <string>
#include <stdio.h>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "myhttp.hh"
#include "dirbrowser.hh"
using namespace std;


void loadFile(string asset, HTTPResponse* httpRes){
	FILE* f;
	f = fopen(asset.c_str(),"r");
	fseek(f,0,SEEK_END);
	httpRes->_bodySize = ftell(f);
	httpRes->_body = new char[httpRes->_bodySize];
	rewind(f);
	fread(httpRes->_body, sizeof(char),httpRes->_bodySize,f);
}

void DirBrowser::serveAsset(string asset, HTTPResponse* httpRes){
	struct stat attr;
	stat(asset.c_str(), &attr);
	if(S_ISREG(attr.st_mode)){
		loadFile(asset,httpRes);
	}
	if(S_ISDIR(attr.st_mode)){
		/*  */
	}
}

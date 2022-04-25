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
	/* */
}

void DirBrowser::serveAsset(string asset, HTTPResponse* httpRes){
	struct stat attr;
	stat(asset.c_str(),&attr);
	if(S_ISREG(attr.st_mode)){
		loadFile(asset,httpRes);
	}
	if(S_ISDIR(attr.st_mode)){
		/*  */
	}
}

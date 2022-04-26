#include <string>
#include <stdio.h>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sstream>
#include <iostream>
#include <ctrsing>
#include "myhttp.hh"
#include "dirbrowser.hh"
using namespace std;

string topDirTemplate = 
	"<!DOCTYPE HTML>"
	"<html>"
	"<head>dir-name</head>"
	"<body>"
	"<h1>dir-name</h1>"
	"<table>"
	"<tr><th valign=\"top\"><img src=\"/icons/blank.gif\" alt=\"[ICO]\"></th><th><a href=\"?C=N;O=D\">Name</a></th><th><a href=\"?C=M;O=A\">Last modified</a></th><th><a href=\"?C=S;O=A\">Size</a></th><th><a href=\"?C=D;O=A\">Description</a></th></tr>";

string bottomDirTemplate = 
	"<tr><th colspan=\"5\"><hr></th></tr>"
	"</table>"
	"</body>"
	"</html>";

DirEntry::DirEntry(string fname, struct stat fattr) {
	_name = fname;
	_modified = fattr.st_mtime;
	_description = string("");
	_size = (long int) fattr.st_size;
	_type = S_ISDIR(fattr.st_mode) ? dir : file;

}

string DirEntry::toString(){
	stringstream ss;
	ss << "<tr>";
	ss << "<td>" << _name << "</td>";
	ss << "<td>" << to_string(_size) << "</td>";
	ss << "<td>" << string(ctime(&_modified)) << "</td>";
	ss << "</tr>";
	return ss.str();

}
void loadFile(string asset, HTTPResponse* httpRes){
	FILE* f;
	f = fopen(asset.c_str(),"r");
	fseek(f,0,SEEK_END);
	httpRes->_bodySize = ftell(f);
	httpRes->_body = new char[httpRes->_bodySize];
	rewind(f);
	fread(httpRes->_body, sizeof(char),httpRes->_bodySize,f);
}

string assembleHTML(vector<DirEntry*> entries) {
	string templateHTML;
	string entryHTML;
	int entriesInd;
	templateHTML = string(topDirTemplate);
	entriesInd = templateHTML.find("<entries>");
	
	for(DirEntry* entr : entries){
		entryHTML += entr->toString();
		delete entr;
	}
	templateHTML += entryHTML;
	templateHTML += bottomDirTemplate;
	return templateHTML;
}

void loadDire(string asset,HTTPResponse* httpRes){	
	DIR* dir;
	struct dirent* ent;
	string fname;
	string path;
	struct stat fattr;
	vector<DirEntry*> entries;
	string rawHTML;
	if((dir = opendir(asset.c_str())) == NULL){
		perror("opendir");
		exit(-1);
	}

	while((ent = readdir(dir)) != NULL){
		path = asset;
		path += "/";
		fname = string(ent->d_name);
		path += fname;
		stat(path.c_str(),&fattr);
		entries.push_back(new DirEntry(fname,fattr));
	}	
	rawHTML = assembleHTML(entries);
	httpRes->_bodySize = rawHTML.length(); 
	httpRes->_body = new char[httpRes->_bodySize];
	strcpy(httpRes->_body,rawHTML.c_str());
}

void DirBrowser::serveAsset(string asset, HTTPResponse* httpRes){
	struct stat attr;
	stat(asset.c_str(), &attr);
	if(S_ISREG(attr.st_mode)){
		loadFile(asset,httpRes);
	}
	if(S_ISDIR(attr.st_mode)){
		loadDire(asset,httpRes);
	}
}

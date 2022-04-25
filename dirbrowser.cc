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
#include "myhttp.hh"
#include "dirbrowser.hh"
using namespace std;


DirEntry::DirEntry(string fname, struct stat fattr) {
	_name = fname;
	_modified = fattr.st_mtime;
	_description = string("");
	_size = (long long) fattr.st_size;
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

char* assembleHTML(vector<DirEntry*> entries) {
	stringstream ss;
	for(DirEntry* entr : entries){
		ss << entr->toString();
	}
}

void loadDire(string asset,HTTPResponse* httpRes){	
	DIR* dir;
	struct dirent* ent;
	string fname;
	string path;
	struct stat fattr;
	vector<DirEntry*> entries;
	if((dir = opendir(asset.c_str())) == NULL){
		perror("opendir");
		exit(-1);
	}

	while((ent = readdir(dir)) != NULL){
		path = asset;
		fname = string(ent->d_name);
		path += fname;
		stat(path.c_str(),&fattr);
		entries.push_back(new DirEntry(fname,fattr));
	}

	assembleHTML(entries);
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

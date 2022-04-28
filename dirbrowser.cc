#include <string>
#include <stdio.h>
#include <functional>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <algorithm>   
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
	"<tr><th valign=\"top\"><img src=\"/icons/index.gif\" alt=\"[ICO]\"></th><th><a href=\"?C=N;O=D\">Name</a></th><th><a href=\"?C=M;O=A\">Last modified</a></th><th><a href=\"?C=S;O=A\">Size</a></th><th><a href=\"?C=D;O=A\">Description</a></th></tr>";
	
string bottomDirTemplate = 
	"<tr><th colspan=\"5\"><hr></th></tr>"
	"</table>"
	"</body>"
	"</html>";


string getIcon(string fname, enum type _type){
	int extIdx;
	string ext;
	if(fname == string(".")){
		return string("");
	}
	if(fname == string("..")){
		return string("/icons/red_ball.gif");
	}
	if(_type == dir){
		return string("/icons/menu.gif");
	}
	if((extIdx = fname.find(".")) == string::npos){
		return string("/icons/binary.gif");
	}
	ext = fname.substr(extIdx, fname.length() - extIdx);
	if(ext == string(".gif") || ext == string(".png") || ext == string(".xbm")){
		return string("/icons/image.gif");
	}
	else{
		return string("/icons/text.gif");
	}	

}
string getParent(string asset){
	string res;
	int indexDouble = 0;
	int slashCnt = 0;
	int idxParent = 0;
	if((indexDouble = asset.find("//")) != string::npos){
		asset.replace(indexDouble,2,"/");
	}
	for(size_t i = asset.length() - 1; i >= 0; i++){
		if(asset.at(i) == '/'){
			slashCnt ++;
		}
		if(slashCnt == 2){
			idxParent = i;
			break;
		}		
	}
	return asset.substr(0,idxParent);
}
DirEntry::DirEntry(string fname,string asset, struct stat fattr) {
	_name = fname;
	_modified = fattr.st_mtime;
	_description = string("");
	_size = (long int) fattr.st_size;
	_type = S_ISDIR(fattr.st_mode) ? dir : file;
	_icon = getIcon(fname,_type);
  if(asset.find(string("..")) != string::npos){
		_path = getParent(asset);
	}
	else{
		_path = asset;
	}
}

bool compTimeNeg(DirEntry* ent1, DirEntry* ent2){
	return !(ent1->_modified < ent2->_modified);
}
bool compTime(DirEntry* ent1, DirEntry* ent2){
	return ent1->_modified < ent2->_modified;
}
bool compSize(DirEntry* ent1, DirEntry* ent2){
	return ent1->_size < ent2->_size;
}
bool compSizeNeg(DirEntry* ent1, DirEntry* ent2){
	return !(ent1->_size < ent2->_size);
}
bool compName(DirEntry* ent1, DirEntry* ent2){
		return ent1->_name < ent2->_name;
}
bool compNameNeg(DirEntry* ent1, DirEntry* ent2){
	return !(ent1->_name < ent2->_name);
}
bool compDesc(DirEntry* ent1, DirEntry* ent2){
	return ent1->_description < ent2->_description;
}

string DirEntry::toString(){
	stringstream ss;
	ss << "<tr><td valign=\"top\">";
	ss << "<img src=\"" << _icon << "\"></td>";
	ss << "<td>" << "<a href=\"" << _path << "\">";
	ss	<< _name << "</a></td>";
	ss << "<td>" << string(ctime(&_modified)) << "</td>";	
	ss << "<td>&nbsp;</td>";
	ss << "<td>" << to_string(_size) << "</td>";	
	ss << "<td>&nbsp;</td>";
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
		if(entr->_name != string(".")){
			entryHTML += entr->toString();
		}
		delete entr;
	}
	templateHTML += entryHTML;
	templateHTML += bottomDirTemplate;
	return templateHTML;
}

vector<DirEntry*> sortBy(vector<DirEntry*> entries,vector<string> params){
	if(params.size() == 0){
		sort(entries.begin(),entries.end(),compName);
		return entries;
	}
	char sortType = params.at(0).at(2);
	char sortOrder = params.at(1).at(2);
	switch(sortType){
		case 'N':
			sort(entries.begin(),entries.end(),compName);
			break;
		case 'S':
			sort(entries.begin(),entries.end(), compSize);
			break;
		case 'M':
			sort(entries.begin(), entries.end(), compTime);
			break;
		case 'D':
			sort(entries.begin(),entries.end(), compDesc);
			break;
		default:
			sort(entries.begin(),entries.end(),compName);
			return entries;		
	}
	switch(sortOrder){
		case 'D':
			reverse(entries.begin(),entries.end());
			break;
		default:
			break;
	}	

	return entries;
}

void loadDire(string asset,HTTPResponse* httpRes, vector<string> params){	
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
		path.erase(0,string("http-root-dir").length());
		entries.push_back(new DirEntry(fname,path,fattr));
	}	
	entries = sortBy(entries,params);	
	rawHTML = assembleHTML(entries);
	httpRes->_bodySize = rawHTML.length(); 
	httpRes->_body = new char[httpRes->_bodySize + 1];
	strcpy(httpRes->_body,rawHTML.c_str());
	closedir(dir);
}

void DirBrowser::serveAsset(string asset, HTTPResponse* httpRes,vector<string> params){
	struct stat attr;
	stat(asset.c_str(), &attr);
	int8_t paramInt;
	if(S_ISREG(attr.st_mode)){
		loadFile(asset,httpRes);
	}
	if(S_ISDIR(attr.st_mode)){
		loadDire(asset,httpRes,params);
	}
}

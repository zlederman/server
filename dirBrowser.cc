#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "dirBrowser.hh"

using namespace std;
DirEntry::DirEntry(string fname, struct stat fattr){
	_name = fname;
	_size = fattr.st_size;
	_modified = fattr.st_mtime;
}


char* DirBrowser::buildHTML(string path,enum sortBy by, enum sortOrder order){
	vector<DirEntry*> entries;
	
}

char* serveAsset(string asset){


}

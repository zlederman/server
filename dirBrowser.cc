#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "dirBrowser.hh"


DirEntry::DirEntry(struct stat attr, string fname){
	_name = fname;
	_size = attr.st_size;
	_modified = attr.st_mtime;
}


DirBrowser::buildHTML(string path,enum sortBy by, enum sortOrder order){
	vector<DirEntry*> entries;
	
}

CXX = g++ -g -fPIC
NETLIBS= -lnsl

all: git-commit myhttpd use-dlopen hello.so

myhttp.o: myhttp.cc myhttp.hh
	$(CXX) -c myhttp.cc $(NETLIBS)

dirbrowser.o: dirbrowser.cc dirbrowser.hh
	$(CXX) -c dirbrowser.cc $(NETLIBS)

logger.o: logger.cc logger.hh
	$(CXX) -pthread -c logger.cc $(NETLIBS)

myhttpd : myhttpd.o myhttp.o dirbrowser.o logger.o
	$(CXX) -pthread -o $@ $@.o dirbrowser.o logger.o  myhttp.o $(NETLIBS)

use-dlopen: use-dlopen.o
	$(CXX) -o $@ $@.o $(NETLIBS) -ldl

hello.so: hello.o
	ld -G -o hello.so hello.o

%.o: %.cc
	@echo 'Building $@ from $<'
	$(CXX) -o $@ -c -I. $<

.PHONY: git-commit
git-commit:
	git checkout
	git add *.cc *.h Makefile >> .local.git.out  || echo
	git commit -a -m 'Commit' >> .local.git.out || echo
	git push origin master 

.PHONY: clean
clean:
	rm -f *.o use-dlopen hello.so
	rm -f *.o daytime-server myhttpd


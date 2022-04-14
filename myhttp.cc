#include <string>
#include <stdio>
#include <regex>

using namespace std;
enum messageType { GET, POST };
enum responseCode {a };
class HTTPMessageFactory {
	public: 
		static HTTPMessage getMessage(string raw);

};


static HTTPMessage HTTPMessageFactory::getMessage(string raw){
			cout << raw << endl;	
			return;
}

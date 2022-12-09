#include <string>
#include <iostream>
using namespace std;
int main() {
    string t = "abcd\r\nefgh\r\n\r\nhijklmn\r\n";
    cout << t.find("\r\n\r\n") << '\n';
    cout << t.substr(0, 10) << '\n';
    return 0;
}
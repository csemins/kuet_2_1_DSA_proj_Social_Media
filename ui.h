#ifndef UI_H
#define UI_H


//Terminal UI helpers (ANSI colours, termios, prompts)
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <termios.h>
#include <unistd.h>
using namespace std;

namespace C {
    const char* R = "\033[0m";
    const char* B = "\033[1m";
    const char* D = "\033[2m";
    const char* I = "\033[3m";
    const char* RED = "\033[31m";
    const char* GREEN = "\033[32m";
    const char* YELLOW = "\033[33m";
    const char* BLUE = "\033[34m";
    const char* MAG = "\033[35m";
    const char* CYAN = "\033[36m";
    const char* WHITE = "\033[37m";
}

inline void clearScreen() { system("clear"); }

inline int tw() {
    const char* c = getenv("COLUMNS");
    return c ? atoi(c) : 80;
}

inline void rule(char ch = '-') {
    int w = tw();
    cout << C::D;
    for (int i = 0; i < w; i++) cout << ch;
    cout << C::R << "\n";
}

inline void centred(const string& s) {
    int w = tw(), pad = (w - (int)s.size()) / 2;
    for (int i = 0; i < pad; i++) cout << ' ';
    cout << s << "\n";
}

inline void ok(const string& m) { cout << C::GREEN << "  ✓  " << m << C::R << "\n"; }
inline void err(const string& m) { cout << C::RED << "  ✗  " << m << C::R << "\n"; }
inline void info(const string& m) { cout << C::CYAN << "  ℹ  " << m << C::R << "\n"; }
inline void warn(const string& m) { cout << C::YELLOW << "  ⚠  " << m << C::R << "\n"; }

inline string promptLine(const string& msg) {
    cout << C::YELLOW << "  › " << msg << C::R;
    string line; getline(cin, line); return line;
}

inline int promptInt(const string& msg) {
    string s = promptLine(msg);
    try { return stoi(s); } catch (...) { return -9999; }
}

inline void waitEnter() {
    cout << C::D << "\n  [Enter to continue…]" << C::R;
    string t; getline(cin, t);
}

inline string readPassword(const string& msg) {
    cout << C::YELLOW << "  › " << msg << C::R << flush;
    if (!isatty(STDIN_FILENO)) { string p; getline(cin, p); cout << "\n"; return p; }

    termios old; tcgetattr(STDIN_FILENO, &old);
    termios nw = old; nw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &nw);
    string pass; char ch;
    while (read(STDIN_FILENO, &ch, 1) == 1 && ch != '\n' && ch != '\r') {
        if (ch == 127 || ch == 8) {
            if (!pass.empty()) { pass.pop_back(); cout << "\b \b" << flush; }
        } else {
            pass.push_back(ch); cout << '*' << flush;
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    cout << "\n"; return pass;
}

inline string nowStamp() {
    time_t t = time(nullptr); char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&t));
    return string(buf);
}

#endif

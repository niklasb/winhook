#include <iostream>
#include <windows.h>
using namespace std;
int foo(int x) {
    cout << "foo: " << x << endl;
    return x;
}

void bar(int y) {
    cout << "fuu: " << y << endl;
}

struct Player {
    int d;
    int __thiscall test(int a, int b, int c) {
        cout << a << " " << b << " " << c << " " << d << endl;
        return a + b + c + d;
    }
};

int main() {
    int x = 1;
    Player p;
    int a, b, c;
    cin >> a >> b >> c >> p.d;
    for(;;) {
        bar(x);
        cout << "bar: " << foo(x) << endl;
        cout << p.test(a,b,c) << endl;
        cin.get();
        if (cin.eof()) break;
        x++;
    }
}
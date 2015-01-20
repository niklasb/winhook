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
    virtual int test(int a, int b, int c) {
        cout << a << " " << b << " " << c << " " << d << endl;
        return a + b + c + d;
    }
};

struct A : Player {
	int test(int a, int b, int c) {
        cout << a << " " << b << " " << c << " " << d << endl;
        return a * b * c * d;
    }
};

int main() {
    int x = 1;
    int a, b, c, d;
    cin >> a >> b >> c >> d;
	A aa;
	aa.d = d;
    Player* p = &aa;
    for(;;) {
        bar(x);
        cout << "bar: " << foo(x) << endl;
        cout << p->test(a,b,c) << endl;
        cin.get();
        if (cin.eof()) break;
        x++;
    }
}
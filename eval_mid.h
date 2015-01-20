struct A {};
template <typename T>
struct B {
    const T& t; 
    B(const T& t): t(t) {};
};
template <typename T> B<T> operator<<(A l, const T& r) { return B<T>(r); }
template <typename T, typename S> T operator<<(B<T> l, S r) { return l.t; }
#define GET_LEFT(a, b) (A() << (a) << (b))
#define GET_RIGHT(a, b) (a,b)
#define GET_MID(a, b, c) GET_RIGHT(a, GET_LEFT(b, c))
#include <iostream.h>

enum bob{a,b,c};

struct point {
        bob x, y;
        point (bob aa, bob bb) : x(aa),y(bb) { }
};


int main() {
        point blah(bob a, bob b);
}


#include "slidingwindow.hpp"

#include <iostream>

int main() {
    SlidingWindow<int> window(4);

    auto check = [&](){ std::cout << window.next_id() << std::endl;
        if (window.ready()) { std::cout << "got " << *window.pop() << std::endl; }
     };
    check();

    window.insert(0,40);
    check();

    window.insert(1,41);
    window.insert(2,42);
    check();
    check();

    window.insert(4,44);
    check();
    window.insert(3,43);
    check();
    check();
    

    return 0;
}
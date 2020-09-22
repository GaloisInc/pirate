#include "ratelimit.hpp"
#include <iostream>
#include <string>
int main () {
    auto cost = std::chrono::duration_cast<RateLimit::clock::duration>(std::chrono::seconds(2));
    RateLimit r(cost,5);

    for(;;) {
        std::string line;
        std::getline(std::cin, line);
        
        auto delay = r.delay_needed();
        if (delay > RateLimit::clock::duration(0)) {

            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(delay);
            std::cout << millis.count() << std::endl;
        } else {
            r.mark();
            std::cout << "MARK" << std::endl;
        }
    }

    return 0;
}
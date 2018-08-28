#include <stdio.h>
#include <vector>
#include <memory>

#include "core.h"

int main(int argc, char** argv) {
    printf("creating CVC\n");
    std::unique_ptr<DecisionEngine> d;
    std::vector<std::unique_ptr<Character>> c;
    c.push_back(std::unique_ptr<Character>());
    c.push_back(std::unique_ptr<Character>());

    CVC cvc(std::move(d), std::move(c));

    printf("running the game loop\n");
    cvc.GameLoop();

    return 0;
}

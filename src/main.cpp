#include <stdio.h>
#include <vector>
#include <memory>
#include <random>

#include "core.h"

int main(int argc, char** argv) {
    printf("creating CVC\n");
    std::unique_ptr<DecisionEngine> d;
    std::vector<std::unique_ptr<Character>> c;
    c.push_back(std::make_unique<Character>(1, 100.0));
    c.push_back(std::make_unique<Character>(2, 100.0));

    std::random_device rd;
    std::mt19937 random_generator(rd());

    CVC cvc(std::move(d), std::move(c), random_generator);

    printf("running the game loop\n");
    cvc.GameLoop();

    return 0;
}

#include <stdio.h>
#include <memory>
#include <random>
#include <vector>

#include "core.h"

int main(int argc, char** argv) {
  printf("creating CVC\n");
  std::unique_ptr<DecisionEngine> d;
  std::vector<std::unique_ptr<Character>> c;
  c.push_back(std::make_unique<Character>(1, 100.0));
  c.push_back(std::make_unique<Character>(2, 100.0));
  c.push_back(std::make_unique<Character>(3, 100.0));

  std::random_device rd;
  std::mt19937 random_generator(rd());

  FILE* action_log = fopen("/tmp/action_log", "a");
  CVC cvc(std::move(d), std::move(c), random_generator, action_log);

  printf("running the game loop\n");
  cvc.GameLoop();

  fclose(action_log);

  return 0;
}

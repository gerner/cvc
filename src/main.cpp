#include <stdio.h>
#include <memory>
#include <random>
#include <vector>

#include "core.h"
#include "decision_engine.h"
#include "action_factories.h"

int main(int argc, char** argv) {
  printf("creating CVC\n");
  std::vector<std::unique_ptr<Character>> c;
  c.push_back(std::make_unique<Character>(1, 100.0));
  c.push_back(std::make_unique<Character>(2, 100.0));
  c.push_back(std::make_unique<Character>(3, 100.0));

  std::random_device rd;
  std::mt19937 random_generator(rd());

  FILE* action_log = fopen("/tmp/action_log", "a");
  CVC cvc(std::move(c), random_generator);

  GiveActionFactory gaf;
  AskActionFactory aaf;
  TrivialActionFactory taf;

  DecisionEngine d({&gaf, &aaf, &taf}, &cvc, action_log);

  printf("running the game loop\n");
  d.GameLoop();

  fclose(action_log);

  return 0;
}

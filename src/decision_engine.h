#ifndef DECISION_ENGINE_H_
#define DECISION_ENGINE_H_

#include "core.h"
#include "action.h"

class DecisionEngine {
 public:
  DecisionEngine(CVC* cvc, FILE* action_log);
  void GameLoop();

 private:
  std::vector<std::unique_ptr<Action>> EnumerateActions(Character* character);
  void ChooseActions();
  void EvaluateQueuedActions();

  void LogAction(const Action* action);

  CVC* cvc_;
  FILE* action_log_;
  std::vector<std::unique_ptr<Action>> queued_actions_;
};

#endif

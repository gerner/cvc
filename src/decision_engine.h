#ifndef DECISION_ENGINE_H_
#define DECISION_ENGINE_H_

#include "core.h"
#include "action.h"

// Creates and scores action instances for a specific type of action
class ActionFactory {
 public:
  virtual ~ActionFactory();
  //
  virtual double EnumerateActions(
      const CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) = 0;
};

class DecisionEngine {
 public:
  DecisionEngine(std::vector<ActionFactory*> action_factories, CVC* cvc,
                 FILE* action_log);
  void GameLoop();

 private:
  std::vector<std::unique_ptr<Action>> EnumerateActions(
      Character* character);
  void ChooseActions();
  void EvaluateQueuedActions();

  void LogAction(const Action* action);

  const std::vector<ActionFactory*> action_factories_;
  CVC* cvc_;
  FILE* action_log_;
  std::vector<std::unique_ptr<Action>> queued_actions_;
};

#endif

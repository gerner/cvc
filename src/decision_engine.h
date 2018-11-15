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
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) = 0;
};

struct Agent {
  Agent(Character* character, ActionFactory* action_factory)
      : character_(character), action_factory_(action_factory) {}

  Character* character_;
  ActionFactory* action_factory_;
};

struct Experience {
  Agent* agent_;

  //previous action
  Action* action;

  //next action
  Action* next_action;
};

class DecisionEngine {
 public:
  DecisionEngine(std::vector<Agent*> agents, CVC* cvc,
                 FILE* action_log);
  void GameLoop();

 private:
  std::vector<std::unique_ptr<Action>> EnumerateActions(
      Agent* agent);
  void ChooseActions();
  void EvaluateQueuedActions();

  void LogInvalidAction(const Action* action);
  void LogAction(const Action* action);

  const std::vector<Agent*> agents_;
  CVC* cvc_;
  FILE* action_log_;
  std::vector<std::unique_ptr<Action>> queued_actions_;
};

#endif

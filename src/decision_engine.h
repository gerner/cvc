#ifndef DECISION_ENGINE_H_
#define DECISION_ENGINE_H_

#include <memory>
#include <vector>

#include "core.h"
#include "action.h"

// Creates and scores action instances for a specific type of action
class ActionFactory {
 public:
  virtual ~ActionFactory();

  virtual double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) = 0;
  virtual bool Respond(CVC* cvc, const Action* action);

  virtual void Learn(CVC* cvc, const Action* action, const Action* next_action);
};

class ActionPolicy {
  public:
   virtual std::unique_ptr<Action> ChooseAction(
       std::vector<std::unique_ptr<Action>>* actions, CVC* cvc,
       Character* character) = 0;
};

struct Agent {
  Agent(Character* character, ActionFactory* action_factory,
        ActionPolicy* policy)
      : character_(character),
        action_factory_(action_factory),
        policy_(policy) {}

  Character* character_;
  ActionFactory* action_factory_;
  ActionPolicy* policy_;
};

struct Experience {
  Agent* agent_;

  //s, a, r, s', a'
  //need:
  //Q(s, a): estimate of final score given prior state/action
  //r: observed reward taking a in state s
  //Q(s', a'): estimate of final score given next state/action

  //previous action
  std::unique_ptr<Action> action_;

  //next action
  std::unique_ptr<Action> next_action_;
};

class DecisionEngine {
 public:
  static std::unique_ptr<DecisionEngine> Create(std::vector<Agent*> agents,
                                                CVC* cvc, FILE* action_log);

  DecisionEngine(std::vector<Agent*> agents, CVC* cvc,
                 FILE* action_log);
  void GameLoop();

 private:
  void ChooseActions();
  void EvaluateQueuedActions();
  void Learn();

  void LogInvalidAction(const Action* action);
  void LogAction(const Action* action);

  const std::vector<Agent*> agents_;
  CVC* cvc_;
  FILE* action_log_;

  // the set of experiences for the current game tick
  // each of these represent the most recent experience for each agent, even the
  // trivial experience, each consisting of the most recent action, the most
  // recent reward and the next action that agent will take.
  std::vector<std::unique_ptr<Experience>> experiences_;

  // a lookup from a character to the decision making capacity for that
  // character, the agent controlling that character.
  // this lookup MUST be maintained in the face of characters entering or
  // leaving the game.
  std::unordered_map<Character*, Agent*> agent_lookup_;
};

#endif

#ifndef DECISION_ENGINE_H_
#define DECISION_ENGINE_H_

#include <memory>
#include <vector>
#include <list>

#include "core.h"
#include "action.h"

class Agent;

struct Experience {
  Experience(Agent* agent, std::unique_ptr<Action> action, Action* next_action)
      : agent_(agent), action_(std::move(action)), next_action_(next_action) {}

  Agent* agent_;

  //s, a, r, s', a'
  //need:
  //Q(s, a): estimate of final score given prior state/action
  //r: observed reward taking a in state s
  //Q(s', a'): estimate of final score given next state/action

  //action this experience represents
  std::unique_ptr<Action> action_;

  //pointer to next action
  Action* next_action_;
};

class ActionLearner {
 public:
  virtual ~ActionLearner();
  virtual void Learn(CVC* cvc, std::unique_ptr<Experience> experience);
};

// Creates and scores action instances for a specific type of action
class ActionFactory {
 public:
  virtual ~ActionFactory();

  virtual double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) = 0;

};

class ResponseFactory {
 public:
  virtual ~ResponseFactory();
  virtual double Respond(CVC* cvc, Character* character, Action* action,
                         std::vector<std::unique_ptr<Action>>* actions);
};

class ActionPolicy {
  public:
   virtual std::unique_ptr<Action> ChooseAction(
       std::vector<std::unique_ptr<Action>>* actions, CVC* cvc,
       Character* character) = 0;
};

// An agent acts on behalf of a character in CVC
// It must be able to, given the current game state choose an "independent"
// action representing what the character will do next
// It must be able to, given the current game state and another character's
// proposal choose a "response" action
// It should learn from experiences during a game tick
class Agent {
 public:
  Agent(Character* character) : character_(character) {}

  /*Agent(Character* character, ActionFactory* action_factory,
        ResponseFactory* response_factory, ActionPolicy* policy)
      : character_(character),
        action_factory_(action_factory),
        response_factory_(response_factory),
        policy_(policy) {}*/

  virtual ~Agent() {}

  // lifecycle:
  // ChooseAction/Respond -> Action -> action plays out -> Experience -> Learn
  virtual std::unique_ptr<Action> ChooseAction(CVC* cvc) = 0;
  virtual std::unique_ptr<Action> Respond(CVC* cvc, Action* action) = 0;
  virtual void Learn(CVC* cvc, std::unique_ptr<Experience> experience) = 0;

  Character* GetCharacter() const { return character_; }

 //protected:

  Character* character_;
  /*ActionFactory* action_factory_;
  ResponseFactory* response_factory_;
  ActionPolicy* policy_;*/
};

struct ExperienceByAgent {
  bool operator() (const Agent* lhs, const std::unique_ptr<Experience>& rhs) {
    return lhs < rhs->agent_;
  }

  bool operator() (const std::unique_ptr<Experience>& lhs, const Agent* rhs) {
    return lhs->agent_ < rhs;
  }

  bool operator() (const std::unique_ptr<Experience>& lhs, const std::unique_ptr<Experience>& rhs) {
    return lhs->agent_ < rhs->agent_;
  }
};

class DecisionEngine {
 public:
  static std::unique_ptr<DecisionEngine> Create(std::vector<Agent*> agents,
                                                CVC* cvc, FILE* action_log);

  DecisionEngine(std::vector<Agent*> agents, CVC* cvc,
                 FILE* action_log);

  // Runs one loop of the game
  // When this method returns a few things will be true
  //    * pending actions are carried out on behalf of characters
  //    * game state ticks forward, any passive activies are carried out
  //    * new actions are chosen on behalf of characters, according to
  //    configured agents
  //    * those agents are given a chance to learn from experiences
  void RunOneGameLoop();

 private:
  void ChooseActions();
  void EvaluateQueuedActions();
  void Learn();

  void LogInvalidAction(const Action* action);
  void LogAction(const Action* action);

  const std::vector<Agent*> agents_;
  CVC* cvc_;
  FILE* action_log_;

  // represents the next set of actions we're going to take
  // note, these are partial experiences which haven't played out and don't
  // have a next action assigned
  std::list<std::unique_ptr<Experience>> queued_actions_;

  // the set of experiences for the current game tick
  // there might be zero or more per agent
  std::vector<std::unique_ptr<Experience>> experiences_;

  // a lookup from a character to the decision making capacity for that
  // character, the agent controlling that character.
  // this lookup MUST be maintained in the face of characters entering or
  // leaving the game.
  std::unordered_map<Character*, Agent*> agent_lookup_;
};

#endif

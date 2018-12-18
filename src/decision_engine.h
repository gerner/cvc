#ifndef DECISION_ENGINE_H_
#define DECISION_ENGINE_H_

#include <memory>
#include <vector>
#include <list>
#include <functional>

#include "core.h"
#include "action.h"

class Agent;

struct Experience {
  static std::unique_ptr<Experience> WrapAction(std::unique_ptr<Action> action);

  Experience(Agent* agent, void* data,
             std::unique_ptr<Action> action, Action* next_action)
      : agent_(agent),
        data_(data),
        action_(std::move(action)),
        next_action_(next_action) {}

  Agent* agent_;
  //data the agent can assoicate with this experience for future reference
  void* data_;

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

// Creates and scores action instances for a specific type of action
class ActionFactory {
 public:
  virtual ~ActionFactory() {}

  virtual double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) = 0;

};

class ResponseFactory {
 public:
  virtual ~ResponseFactory() {}
  virtual double Respond(
      CVC* cvc, Character* character, Action* action,
      std::vector<std::unique_ptr<Experience>>* actions) = 0;
};

class ActionPolicy {
  public:
   virtual std::unique_ptr<Experience> ChooseAction(
       std::vector<std::unique_ptr<Experience>>* actions, CVC* cvc,
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

  virtual ~Agent() {}

  // contract:
  // both ChooseAction and Respond create an action
  // at some point in the future we'll get a callback to learn from the
  // experience of that action playing out
  // after the Learn call, the game no longer needs the experience, so it's up
  // to us to manage it. this is true for the contained action as well.
  virtual std::unique_ptr<Experience> ChooseAction(CVC* cvc) = 0;
  virtual std::unique_ptr<Experience> Respond(CVC* cvc, Action* action) = 0;
  virtual void Learn(CVC* cvc, std::unique_ptr<Experience> experience) = 0;

  Character* GetCharacter() const { return character_; }

 protected:

  Character* character_;
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

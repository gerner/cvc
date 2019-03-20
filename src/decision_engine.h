#ifndef DECISION_ENGINE_H_
#define DECISION_ENGINE_H_

#include <memory>
#include <vector>
#include <list>
#include <functional>

#include "core.h"
#include "action.h"

class Agent;

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
  // both ChooseAction and Respond need to create an action
  // those actions need to live until the next Learn call
  // after that the game engine is done
  // after the Learn call, the game no longer needs the experience, so it's up
  // to us to manage it. this is true for the contained action as well.
  virtual Action* ChooseAction(CVC* cvc) = 0;
  virtual Action* Respond(CVC* cvc, Action* action) = 0;
  virtual void Learn(CVC* cvc) = 0;
  virtual double Score(CVC* cvc) = 0;

  Character* GetCharacter() const { return character_; }

 protected:

  Character* character_;
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
  std::list<Action*> queued_actions_;

  // a lookup from a character to the decision making capacity for that
  // character, the agent controlling that character.
  // this lookup MUST be maintained in the face of characters entering or
  // leaving the game.
  std::unordered_map<Character*, Agent*> agent_lookup_;
};

#endif

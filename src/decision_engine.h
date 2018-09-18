#ifndef DECISION_ENGINE_H_
#define DECISION_ENGINE_H_

#include "core.h"

class Action {
 public:
  Action(Character* actor, double score, std::vector<double> features);
  virtual ~Action();
  // Get this action's actor (the character taking the action)
  Character* GetActor() const;
  void SetActor(Character* actor);

  std::vector<double> GetFeatureVector() const;
  void SetFeatureVector(std::vector<double> feature_vector);

  // Get the score this action's been given
  // it's assumed this is normalized against alternative actions
  // I'm not thrilled with this detail which makes it make sense ONLY in the
  // context of some other instances
  double GetScore() const;
  void SetScore(double score);

  // Determine if this particular action is valid in the given gamestate by
  // the given character
  virtual bool IsValid(const CVC* gamestate) = 0;

  // Have this action take effect by the given character
  virtual void TakeEffect(CVC* gamestate) = 0;

 private:
  std::vector<double> feature_vector_;
  Character* actor_;
  double score_;
};

class DecisionEngine {
 public:
  DecisionEngine(CVC* cvc, FILE* action_log);
  void GameLoop();

 private:
  std::vector<std::unique_ptr<Action>> EnumerateActions(Character* character);
  void ChooseActions();
  void EvaluateQueuedActions();

  void LogAction(const Action* action);

  FILE* action_log_;
  std::vector<std::unique_ptr<Action>> queued_actions_;
  CVC* cvc_;
};

#endif

#ifndef DECISION_ENGINE_H_
#define DECISION_ENGINE_H_

#include "core.h"

class Action {
 public:
  Action(const char* action_id, Character* actor, double score, std::vector<double> features);
  virtual ~Action();
  // Get this action's actor (the character taking the action)
  Character* GetActor() const {
    return actor_;
  }
  void SetActor(Character* actor) {
    actor_ = actor;
  }

  std::vector<double> GetFeatureVector() const {
    return feature_vector_;
  }
  void SetFeatureVector(std::vector<double> feature_vector) {
    feature_vector_ = feature_vector;
  }

  // Get the score this action's been given
  // it's assumed this is normalized against alternative actions
  // I'm not thrilled with this detail which makes it make sense ONLY in the
  // context of some other instances
  double GetScore() const {
    return score_;
  }
  void SetScore(double score) {
    score_ = score;
  }

  const char* GetActionId() const {
    return action_id_;
  }

  // Determine if this particular action is valid in the given gamestate by
  // the given character
  virtual bool IsValid(const CVC* gamestate) = 0;

  // Have this action take effect by the given character
  virtual void TakeEffect(CVC* gamestate) = 0;

 private:
  const char* action_id_;
  Character* actor_;
  double score_;
  std::vector<double> feature_vector_;

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

  CVC* cvc_;
  FILE* action_log_;
  std::vector<std::unique_ptr<Action>> queued_actions_;
};

#endif

#ifndef ACTION_H_
#define ACTION_H_

#include <vector>

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

class TrivialAction : public Action {
 public:
  TrivialAction(Character* actor, double score, std::vector<double> features);

  // implementation of Action
  bool IsValid(const CVC* gamestate);
  void TakeEffect(CVC* gamestate);
};

// AskAction: ask target character for money, they accept with some chance
// increasing with opinion
class AskAction : public Action {
 public:
  AskAction(Character* actor, double score, std::vector<double> features,
            Character* target, double request_amount);

  // implementation of Action
  bool IsValid(const CVC* gamestate);
  void TakeEffect(CVC* gamestate);

 private:
  Character* target_;
  double request_amount_;
};

// StealAction: (try to) steal money from target character, succeeds with chance
// increasing with opinion, detected with chance increasing with opinion
class StealAction : public Action {
 public:
  StealAction(Character* actor, double score, std::vector<double> features);

  // implementation of Action
  bool IsValid(const CVC* gamestate);
  void TakeEffect(CVC* gamestate);

 private:
  Character* target_;
  double steal_amount_;
};

// GiveAction: give money to target character, increases opinion depending on
// other opinion modifiers
class GiveAction : public Action {
 public:
  GiveAction(Character* actor, double score, std::vector<double> features,
             Character* target, double gift_amount);

  // implementation of Action
  bool IsValid(const CVC* gamestate);
  void TakeEffect(CVC* gamestate);

 private:
  Character* target_;
  double gift_amount_;
};

#endif

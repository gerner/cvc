#ifndef CORE_H_
#define CORE_H_

#include <vector>
#include <memory>
#include <random>

class Character {
  public:
    double GetMoney();
    void SetMoney(double money);

  private:
    double money_;
};

class CVC;

class Action {
  public:
    Action(Character* actor, double score);
    virtual ~Action();
    //Get this action's actor (the character taking the action)
    Character* GetActor();
    void SetActor(Character* actor);

    //Get the score this action's been given
    //it's assumed this is normalized against alternative actions
    //I'm not thrilled with this detail which makes it make sense ONLY in the
    //context of some other instances
    double GetScore();
    void SetScore(double score);

    //Determine if this particular action is valid in the given gamestate by
    //the given character
    virtual bool IsValid(const CVC& gamestate) = 0;

    //Have this action take effect by the given character
    virtual void TakeEffect(CVC& gamestate) = 0;

  private:
    Character* actor_;
    double score_;
};

class TrivialAction : public Action {
  public:
    TrivialAction(Character *actor, double score);

    //implementation of Action
    bool IsValid(const CVC& gamestate);
    void TakeEffect(CVC& gamestate);

};

class DecisionEngine {
  public:
    //TODO: what's the right return type here? the idea is that this is a
    //factory of scored actions in a vector
    std::vector<std::unique_ptr<Action>> EnumerateActions(const CVC& cvc, Character* character);
};

class CVC {
  public:
    CVC(
            std::unique_ptr<DecisionEngine> decision_engine,
            std::vector<std::unique_ptr<Character>> characters,
            std::mt19937 random_generator);

    void GameLoop();

  private:
    void EvaluateQueuedActions();
    void ChooseActions();

    std::mt19937 random_generator_;

    std::unique_ptr<DecisionEngine> decision_engine_;
    std::vector<std::unique_ptr<Character>> characters_;
    std::vector<std::unique_ptr<Action>> queued_actions_;

};

#endif

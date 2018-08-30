#ifndef CORE_H_
#define CORE_H_

#include <vector>
#include <list>
#include <memory>
#include <random>

#include "action.h"

class Action;
class CVC;
class Character;

struct RelationshipModifier {
    RelationshipModifier(Character* target, int start_date, int end_date, double opinion_modifier_);

    Character* target_;
    int start_date_;
    int end_date_;
    double opinion_modifier_;
    //TODO: some explanatory string about why
};

class Character {
  public:
    Character(int id, double money_);
    int GetId() const;
    double GetMoney() const;
    void SetMoney(double money);
    void AddRelationship(std::unique_ptr<RelationshipModifier> relationship);
    void ExpireRelationships(int now);
    double GetOpinionOf(Character *target) const;

  private:
    int id_;
    double money_;
    std::list<std::unique_ptr<RelationshipModifier>> relationships_;

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

    std::vector<Character *> GetCharacters() const;

    void GameLoop();
    void PrintState() const;

    //gets the current clock tick
    int Now() const;

    std::mt19937& GetRandomGenerator();

  private:
    void ExpireRelationships();
    void EvaluateQueuedActions();
    void ChooseActions();

    int ticks_;

    std::mt19937 random_generator_;

    std::unique_ptr<DecisionEngine> decision_engine_;
    std::vector<std::unique_ptr<Character>> characters_;
    std::vector<std::unique_ptr<Action>> queued_actions_;

};

#endif

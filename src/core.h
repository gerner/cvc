#ifndef CORE_H_
#define CORE_H_

#include <vector>
#include <list>
#include <memory>
#include <random>
#include <stdio.h>

class CVC;
class Character;

struct RelationshipModifier {
  RelationshipModifier(Character* target, int start_date, int end_date,
                       double opinion_modifier_);

  Character *target_;
  int start_date_;
  int end_date_;
  double opinion_modifier_;
  // TODO: some explanatory string about why
};

class Character {
 public:
  Character(int id, double money_);

  int GetId() const { return this->id_; }

  double GetMoney() const { return this->money_; }

  void SetMoney(double money) { this->money_ = money; }

  void AddRelationship(std::unique_ptr<RelationshipModifier> relationship);
  void ExpireRelationships(int now);
  double GetOpinionOf(Character* target) const;

 private:
  int id_;
  double money_;
  std::list<std::unique_ptr<RelationshipModifier>> relationships_;

};

class CVC {
 public:
  CVC(std::vector<std::unique_ptr<Character>> characters,
      std::mt19937 random_generator);

  std::vector<Character*> GetCharacters() const;

  void PrintState() const;

  // gets the current clock tick
  int Now() {
    return ticks_;
  }

  void Tick() {
    ticks_++;
  }

  void ExpireRelationships();

  std::mt19937* GetRandomGenerator() { return &this->random_generator_; }

 private:
  int ticks_;
  std::mt19937 random_generator_;
  std::vector<std::unique_ptr<Character>> characters_;

};

#endif

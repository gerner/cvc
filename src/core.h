#ifndef CORE_H_
#define CORE_H_

#include <limits>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <list>
#include <memory>
#include <random>
#include <cstdio>

#include "util.h"

enum CharacterTraitId {
  kBackground,
  kLanguage
};

typedef int CharacterTrait;

typedef int CharacterId;
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
  Character(CharacterId id, double money_);
  CharacterId GetId() const { return this->id_; }

  double GetMoney() const { return this->money_; }
  void SetMoney(double money) { this->money_ = money; }

  double GetScore() const { return this->score_; }
  void SetScore(double score) { this->score_ = score; }

  void AddRelationship(std::unique_ptr<RelationshipModifier> relationship);
  void ExpireRelationships(int now);
  double GetOpinionOf(const Character* target);
  double GetFreshOpinionOf(const Character* target);

  std::unordered_map<CharacterTraitId, CharacterTrait> traits_;

 private:
  int id_;
  // TODO: decide if we want to keep track of birth date/end date
  /*int start_tick_;
  int end_tick_ = std::numeric_limits<int>::max();*/
  double money_;
  double score_;

  std::unordered_map<CharacterId, std::list<std::unique_ptr<RelationshipModifier>>>
      relationships_;

  std::unordered_map<CharacterId, double> opinion_cache;
};

// Holds game state
class CVC {
 public:
  CVC() {}
  CVC(std::vector<Character*> characters,
      Logger *logger, std::mt19937 random_generator);

  std::vector<Character*> GetCharacters() const;

  void LogState();

  //features
  const Stats& GetOpinionStats();
  const Stats& GetOpinionOfStats(CharacterId id);
  const Stats& GetOpinionByStats(CharacterId id);
  const Stats& GetMoneyStats();

  // gets the current clock tick
  int Now() const {
    return ticks_;
  }

  void Tick();

  std::mt19937* GetRandomGenerator() { return &this->random_generator_; }

  Logger* GetLogger() const { return this->logger_; }

  int invalid_actions_;

 private:
  void ExpireRelationships();

  void ComputeStats();

  std::vector<Character*> characters_;
  int ticks_ = 0;
  Logger *logger_;
  std::mt19937 random_generator_;

  Stats global_opinion_stats_;
  std::unordered_map<CharacterId, Stats> opinion_of_stats_;
  std::unordered_map<CharacterId, Stats> opinion_by_stats_;

  Stats global_money_stats_;
};

#endif

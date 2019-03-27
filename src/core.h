#ifndef CORE_H_
#define CORE_H_

#include <cmath>
#include <vector>
#include <unordered_map>
#include <list>
#include <memory>
#include <random>
#include <cstdio>
#include <stdio.h>
#include <stdarg.h>

enum LogLevel {
  TRACE,
  DEBUG,
  INFO,
  WARN,
  ERROR
};

class Logger {
 public:
  Logger() : logger_name_("LOGGER") {}
  Logger(const char* logger_name, FILE* log_sink, LogLevel level)
      : logger_name_(logger_name), log_sink_(log_sink), log_level_(level) {}

  void Log(const LogLevel level, const char* format, ...) {
    if(level >= log_level_) {
      if(log_sink_) {
        fprintf(log_sink_, "%s	", logger_name_);
        va_list args;
        va_start (args, format);
        vfprintf (log_sink_, format, args);
        va_end (args);
      }
    }
  }

  LogLevel GetLogLevel() {
    return log_level_;
  }
  void SetLogLevel(LogLevel level) {
    log_level_ = level;
  }
 private:
  const char* logger_name_;
  FILE *log_sink_ = stderr;
  LogLevel log_level_ = INFO;
};

struct Stats {
  double mean_;
  double stdev_;
  int n_ = 0;
  double sum_ = 0.0;
  double ss_ = 0.0;
  double min_ = std::numeric_limits<double>::max();
  double max_ = std::numeric_limits<double>::min();

  void Clear() {
    n_ = 0;
    sum_ = 0.0;
    ss_ = 0.0;
    min_ = std::numeric_limits<double>::max();
    max_ = std::numeric_limits<double>::min();
  }

  void Update(double datum) {
    sum_ += datum;
    ss_ += datum * datum;
    if(datum < min_) {
      min_ = datum;
    }
    if(datum > max_) {
      max_ = datum;
    }
    n_++;
  }

  void ComputeStats(double sum, double ss, int n) {
    n_ = n;
    mean_ = sum / (double)n_;
    stdev_ = sqrt(ss / (double)n_ - (mean_ * mean_));
  }

  void ComputeStats() {
    ComputeStats(sum_, ss_, n_);
  }

};

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
  int Now() {
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
  int ticks_;
  Logger *logger_;
  std::mt19937 random_generator_;

  Stats global_opinion_stats_;
  std::unordered_map<CharacterId, Stats> opinion_of_stats_;
  std::unordered_map<CharacterId, Stats> opinion_by_stats_;

  Stats global_money_stats_;
};

#endif

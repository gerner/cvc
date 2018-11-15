#include <stdio.h>
#include <cassert>
#include <iterator>
#include <memory>
#include <random>
#include <sstream>
#include <vector>
#include <unordered_map>

#include "action.h"
#include "core.h"

RelationshipModifier::RelationshipModifier(Character* target, int start_date,
                                           int end_date,
                                           double opinion_modifier)
    : target_(target),
      start_date_(start_date),
      end_date_(end_date),
      opinion_modifier_(opinion_modifier) {}

Character::Character(int id, double money) : id_(id), money_(money) {}

void Character::ExpireRelationships(int now) {
  for (auto rel_pair = relationships_.begin(); rel_pair != relationships_.end();
       rel_pair++) {
    bool updated_opinion = false;
    for (auto it = rel_pair->second.begin(); it != rel_pair->second.end();) {
      RelationshipModifier* relationship = it->get();
      if (now >= relationship->end_date_) {
        //printf("expiring %d -> %d (%f)\n", this->GetId(),
        //       relationship->target_->GetId(), relationship->opinion_modifier_);
        it = rel_pair->second.erase(it);
        updated_opinion = true;
      } else {
        it++;
      }
    }

    if (updated_opinion) {
      opinion_cache.erase(rel_pair->first);
    }
  }
}

void Character::AddRelationship(
    std::unique_ptr<RelationshipModifier> relationship) {
  opinion_cache.erase(relationship->target_->GetId());
  this->relationships_[relationship->target_->GetId()].push_back(
      std::move(relationship));
}

double Character::GetOpinionOf(const Character* target) {
  auto cached_opinion = opinion_cache.find(target->GetId());
  if(cached_opinion != opinion_cache.end()) {
    return cached_opinion->second;
  }

  double opinion = 0.0;
  // TODO: come up with some reasonable way to handle non-relationship-modified
  // opinion traits

  //check for compatible backgrounds
  auto background = traits_.find(kBackground);
  if(background != traits_.end()) {
    auto target_background = target->traits_.find(kBackground);
    if(target_background != target->traits_.end()) {
      if(background->second == target_background->second) {
        opinion += 25.0;
      }
    }
  }

  //check for compatible primary language
  auto language = traits_.find(kLanguage);
  if(language != traits_.end()) {
    auto target_language = target->traits_.find(kLanguage);
    if(target_language != target->traits_.end()) {
      if(language->second != target_language->second) {
        opinion -= 50.0;
      }
    }
  }

  //apply relationship modifiers
  auto it = relationships_.find(target->GetId());
  if (relationships_.end() != it) {
    for (const auto& r : it->second) {
      if (r->target_ == target) {
        opinion += r->opinion_modifier_;
      }
    }
  }

  opinion_cache[target->GetId()] = opinion;

  return opinion;
}

CVC::CVC(std::vector<Character*> characters, Logger* logger,
         std::mt19937 random_generator)
    : invalid_actions_(0),
      characters_(characters),
      ticks_(0),
      logger_(logger),
      random_generator_(random_generator) {}

//TODO: don't do this, have the vector of pointers handy
std::vector<Character*> CVC::GetCharacters() const { return characters_; }

void CVC::LogState() {
  logger_->Log(INFO, "tick %d: invalid actions: %d avg money: %f (%f) avg opinion %f (%f)\n", this->ticks_, this->invalid_actions_, GetMoneyStats().mean_, GetMoneyStats().stdev_, GetOpinionStats().mean_, GetOpinionStats().stdev_);
  for (auto character : this->characters_) {
    const Stats& by_stats = GetOpinionByStats(character->GetId());
    const Stats& of_stats = GetOpinionOfStats(character->GetId());
    logger_->Log(INFO, "%d	%f	%f (%f)	%f (%f)\n", character->GetId(),
                 character->GetMoney(), by_stats.mean_, by_stats.stdev_,
                 of_stats.mean_, of_stats.stdev_);
  }
}

const Stats& CVC::GetOpinionStats() {
  if (global_opinion_stats_.n_) {
    return global_opinion_stats_;
  }
  ComputeStats();
  return global_opinion_stats_;
}

const Stats& CVC::GetOpinionOfStats(CharacterId id) {
  if (global_opinion_stats_.n_) {
    return opinion_of_stats_[id];
  }
  ComputeStats();
  return opinion_of_stats_[id];
}

const Stats& CVC::GetOpinionByStats(CharacterId id) {
  if (global_opinion_stats_.n_) {
    return opinion_by_stats_[id];
  }
  ComputeStats();
  return opinion_by_stats_[id];
}

const Stats& CVC::GetMoneyStats() {
  if (global_money_stats_.n_) {
    return global_money_stats_;
  }
  ComputeStats();
  return global_money_stats_;
}


void CVC::ExpireRelationships() {
  for (auto character : this->characters_) {
    character->ExpireRelationships(this->Now());
  }
}

void CVC::ComputeStats() {
  double global_opinion_ss = 0.0;
  double global_opinion_sum = 0.0;
  double global_money_ss = 0.0;
  double global_money_sum = 0.0;
  for (auto character : characters_) {
    double of_ss = 0.0;
    double of_sum = 0.0;
    double by_ss = 0.0;
    double by_sum = 0.0;

    global_money_ss += character->GetMoney() * character->GetMoney();
    global_money_sum += character->GetMoney();

    for (auto target : characters_) {
      //skip self opinion
      if (character->GetId() == target->GetId()) {
        continue;
      }
      double opinion_of = target->GetOpinionOf(character);
      //only count of for global, we'll get the reflexive case later
      global_opinion_ss += opinion_of * opinion_of;
      global_opinion_sum += opinion_of;

      of_ss += opinion_of * opinion_of;
      of_sum += opinion_of;

      // it's convenient to compute this now, the cost is ammortized since it's
      // cached
      double opinion_by = character->GetOpinionOf(target);
      by_ss += opinion_by * opinion_by;
      by_sum += opinion_by;
    }

    opinion_of_stats_[character->GetId()].ComputeStats(of_sum, of_ss,
                                                      characters_.size());
    opinion_by_stats_[character->GetId()].ComputeStats(by_sum, by_ss,
                                                      characters_.size());
  }
  global_opinion_stats_.ComputeStats(
      global_opinion_sum, global_opinion_ss,
      characters_.size() * characters_.size() - characters_.size());
  global_money_stats_.ComputeStats(global_money_sum, global_money_ss,
                                   characters_.size());
}



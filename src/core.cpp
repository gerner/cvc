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

double Character::GetFreshOpinionOf(const Character* target) {
  auto cached_opinion = opinion_cache.find(target->GetId());
  if(cached_opinion != opinion_cache.end()) {
    opinion_cache.erase(cached_opinion);
  }
  return GetOpinionOf(target);
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

std::vector<Character*> CVC::GetCharacters() const { return characters_; }

void CVC::LogState() {
  logger_->Log(INFO, "tick %d: invalid actions: %d avg money: %f (%f) avg opinion %f (%f)\n", this->ticks_, this->invalid_actions_, GetMoneyStats().mean_, GetMoneyStats().stdev_, GetOpinionStats().mean_, GetOpinionStats().stdev_);
  for (auto character : this->characters_) {
    const Stats& by_stats = GetOpinionByStats(character->GetId());
    const Stats& of_stats = GetOpinionOfStats(character->GetId());
    logger_->Log(INFO, "%d	%f	%f	%f (%f)	%f (%f)\n", character->GetId(),
                 character->GetScore(), character->GetMoney(), by_stats.mean_, by_stats.stdev_,
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

void CVC::Tick() {
  ExpireRelationships();

  //clear cache of stats
  global_opinion_stats_.n_ = 0;
  opinion_of_stats_.clear();
  global_money_stats_.n_ = 0;
  opinion_by_stats_.clear();
  ticks_++;
}


void CVC::ExpireRelationships() {
  for (auto character : this->characters_) {
    character->ExpireRelationships(this->Now());
  }
}

void CVC::ComputeStats() {
  global_opinion_stats_.Clear();
  global_money_stats_.Clear();

  for (auto character : characters_) {
    global_money_stats_.Update(character->GetMoney());
    Stats& opinion_of_stat = opinion_of_stats_[character->GetId()];
    Stats& opinion_by_stat = opinion_by_stats_[character->GetId()];

    for (auto target : characters_) {
      //TODO: convert to Stats::Update and Stats::ComputeStats

      //skip self opinion
      if (character->GetId() == target->GetId()) {
        continue;
      }

      double opinion_of = target->GetFreshOpinionOf(character);
      // it's convenient to compute this now, the cost is ammortized since it's
      // cached
      double opinion_by = character->GetFreshOpinionOf(target);

      //only count of for global, we'll get the reflexive case later
      global_opinion_stats_.Update(opinion_of);
      opinion_of_stat.Update(opinion_of);
      opinion_by_stat.Update(opinion_by);
    }

    opinion_of_stat.ComputeStats();
    opinion_by_stat.ComputeStats();
  }
  global_opinion_stats_.ComputeStats();
  global_money_stats_.ComputeStats();
}

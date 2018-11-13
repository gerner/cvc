#include <stdio.h>
#include <cassert>
#include <iterator>
#include <memory>
#include <random>
#include <sstream>
#include <vector>

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
  for (auto it = this->relationships_.begin();
       it != this->relationships_.end();) {
    RelationshipModifier* relationship = it->get();
    if (now >= relationship->end_date_) {
      //printf("expiring %d -> %d (%f)\n", this->GetId(),
      //       relationship->target_->GetId(), relationship->opinion_modifier_);
      it = this->relationships_.erase(it);
    } else {
      it++;
    }
  }
}

void Character::AddRelationship(
    std::unique_ptr<RelationshipModifier> relationship) {
  this->relationships_.push_back(std::move(relationship));
}

double Character::GetOpinionOf(const Character* target) const {
  double opinion = 0.0;
  // TODO: non-relationship-modified opinion traits

  // TODO: rethink how we're organizing these relationships to make this faster
  for (const auto& r : this->relationships_) {
    if (r->target_ == target) {
      opinion += r->opinion_modifier_;
    }
  }

  return opinion;
}

CVC::CVC(std::vector<std::unique_ptr<Character>> characters,
         std::mt19937 random_generator)
    : ticks_(0), random_generator_(random_generator) {
  this->characters_ = std::move(characters);
}

//TODO: don't do this, have the vector of pointers handy
std::vector<Character*> CVC::GetCharacters() const {
  std::vector<Character*> ret;  //(this->characters_.size());
  for (auto& character : this->characters_) {
    ret.push_back(character.get());
  }
  return ret;
}

void CVC::PrintState() const {
  // TODO: probably don't want to always spit to stdout

  printf("tick %d:\n", this->ticks_);
  for (const auto& character : this->characters_) {
    printf("%d	%f	", character->GetId(), character->GetMoney());
    for (const auto& target : this->characters_) {
      printf("%f	", character->GetOpinionOf(target.get()));
    }
    printf("\n");
  }
}

void CVC::ExpireRelationships() {
  for (const auto& character : this->characters_) {
    character->ExpireRelationships(this->Now());
  }
}



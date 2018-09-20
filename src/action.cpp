#include <cmath>

#include "action.h"
#include "decision_engine.h"
#include "core.h"

TrivialAction::TrivialAction(Character* actor, double score,
                             std::vector<double> features)
    : Action(__FUNCTION__, actor, score, features) {}

bool TrivialAction::IsValid(const CVC* gamestate) { return true; }

void TrivialAction::TakeEffect(CVC* gamestate) {
  printf("trivial by %d\n", GetActor()->GetId());
}

AskAction::AskAction(Character* actor, double score,
                     std::vector<double> features, Character* target,
                     double request_amount)
    : Action(__FUNCTION__, actor, score, features) {
  this->target_ = target;
  this->request_amount_ = request_amount;
}

bool AskAction::IsValid(const CVC* gamestate) {
  return this->target_->GetMoney() > request_amount_;
}

void AskAction::TakeEffect(CVC* gamestate) {
  // check to see if the target will accept
  // opinion < 0 => no, otherwise some distribution improves with opinion
  double opinion = this->target_->GetOpinionOf(this->GetActor());
  std::uniform_real_distribution<> dist(0.0, 1.0);
  bool success = false;
  if (opinion > 0.0 && dist(*gamestate->GetRandomGenerator()) <
                           1.0 / (1.0 + exp(-10.0 * (opinion - 0.5)))) {
    success = true;
  }

  if (success) {
    // on success:
    // transfer request_amount_ from target to actor
    this->GetActor()->SetMoney(this->GetActor()->GetMoney() -
                               this->request_amount_);
    this->target_->SetMoney(this->target_->GetMoney() + this->request_amount_);

    // increase opinion of actor (got money)
    this->GetActor()->AddRelationship(std::make_unique<RelationshipModifier>(
        this->target_, gamestate->Now(), gamestate->Now() + 10,
        this->request_amount_));
    // decrease opinion of target (gave money)
    this->target_->AddRelationship(std::make_unique<RelationshipModifier>(
        this->GetActor(), gamestate->Now(), gamestate->Now() + 10,
        -1.0 * this->request_amount_));

    printf("request by %d to %d of %f\n", this->GetActor()->GetId(),
           this->target_->GetId(), this->request_amount_);
  } else {
    // on failure:
    // decrease opinion of actor (refused request)
    this->GetActor()->AddRelationship(std::make_unique<RelationshipModifier>(
        this->target_, gamestate->Now(), gamestate->Now() + 10,
        this->request_amount_));
    printf("request_failed by %d to %d of %f\n", this->GetActor()->GetId(),
           this->target_->GetId(), this->request_amount_);
  }
}

bool StealAction::IsValid(const CVC* gamestate) {
  return this->target_->GetMoney() > steal_amount_;
}

void StealAction::TakeEffect(CVC* gamestate) {
  // TODO: steal action
  // some distribution improves with opinion

  // on success:
  // transfer steal_amount_ from target to actor
  // decrease opinion of target by a lot (got money stolen)

  // on failure:
  // decrease opinion of actor by  a little bit (tried to get money stolen)
}

GiveAction::GiveAction(Character* actor, double score,
                       std::vector<double> features, Character* target,
                       double gift_amount)
    : Action(__FUNCTION__, actor, score, features) {
  this->target_ = target;
  this->gift_amount_ = gift_amount;
}

bool GiveAction::IsValid(const CVC* gamestate) {
  return this->GetActor()->GetMoney() > gift_amount_;
}

void GiveAction::TakeEffect(CVC* gamestate) {
  // transfer gift_amount_ from actor to target
  this->GetActor()->SetMoney(this->GetActor()->GetMoney() - this->gift_amount_);
  this->target_->SetMoney(this->target_->GetMoney() + this->gift_amount_);

  // increase opinion of target (got money)
  double opinion_buff = this->gift_amount_;
  this->target_->AddRelationship(std::make_unique<RelationshipModifier>(
      this->GetActor(), gamestate->Now(), gamestate->Now() + 10, opinion_buff));

  printf("gift by %d to %d of %f (increase opinion by %f)\n",
         this->GetActor()->GetId(), this->target_->GetId(), this->gift_amount_,
         opinion_buff);
}


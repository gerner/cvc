#include <cmath>
#include <cassert>

#include "action.h"
#include "core.h"

Action::Action(const char* action_id, Character* actor, double score,
               std::vector<double> features)
    : action_id_(action_id),
      actor_(actor),
      target_(NULL),
      score_(score),
      feature_vector_(features) {}

Action::Action(const char* action_id, Character* actor, Character* target,
               double score, std::vector<double> features)
    : action_id_(action_id),
      actor_(actor),
      target_(target),
      score_(score),
      feature_vector_(features) {}

Action::~Action() {}

bool Action::RequiresResponse() {
  return false;
}

TrivialAction::TrivialAction(Character* actor, double score,
                             std::vector<double> features)
    : Action(__FUNCTION__, actor, score, features) {}

bool TrivialAction::IsValid(const CVC* gamestate) { return true; }

void TrivialAction::TakeEffect(CVC* gamestate) {
  SetReward(0.0);
  gamestate->GetLogger()->Log(DEBUG, "trivial by %d\n", GetActor()->GetId());
}

TrivialResponse::TrivialResponse(Character* actor, double score,
                             std::vector<double> features)
    : Action(__FUNCTION__, actor, score, features) {}

bool TrivialResponse::IsValid(const CVC* gamestate) { return true; }

void TrivialResponse::TakeEffect(CVC* gamestate) {
  SetReward(0.0);
  gamestate->GetLogger()->Log(DEBUG, "trivial response by %d\n",
                              GetActor()->GetId());
}

WorkAction::WorkAction(Character* actor, double score,
                             std::vector<double> features)
    : Action(__FUNCTION__, actor, score, features) {}

bool WorkAction::IsValid(const CVC* gamestate) { 
  //there has to be at least one character with positive opinion
  //consider that this character "sponsors" this character
  bool positive_opinion_exists = false;
  for (Character* character : gamestate->GetCharacters()) {
    if (character->GetOpinionOf(GetActor()) > 0.0) {
      positive_opinion_exists = true;
      break;
    }
  }
  return positive_opinion_exists;
}

void WorkAction::TakeEffect(CVC* gamestate) {
  SetReward(1.0);
  GetActor()->SetMoney(GetActor()->GetMoney() + 1.0);
}

AskAction::AskAction(Character* actor, double score,
                     std::vector<double> features, Character* target,
                     double request_amount)
    : Action(__FUNCTION__, actor, target, score, features),
      request_amount_(request_amount) {
  //until we get a positive response, reward is zero
  SetReward(0);
}

bool AskAction::IsValid(const CVC* gamestate) {
  return this->GetTarget()->GetMoney() > request_amount_;
}

bool AskAction::RequiresResponse() {
  return true;
}

void AskAction::TakeEffect(CVC* gamestate) {
  //no effect other than submitting the proposal
  // check to see if the target will accept
  // opinion < 0 => no, otherwise some distribution improves with opinion
  /*double opinion = this->GetTarget()->GetOpinionOf(this->GetActor());
  std::uniform_real_distribution<> dist(0.0, 1.0);
  bool success = false;
  if (opinion > 0.0 && dist(*gamestate->GetRandomGenerator()) <
                           1.0 / (1.0 + exp(-10.0 * (opinion - 0.5)))) {
    success = true;
  }

  if (success) {
    // on success:
    // transfer request_amount_ from target to actor
    this->GetTarget()->SetMoney(this->GetTarget()->GetMoney() -
                                this->request_amount_);
    this->GetActor()->SetMoney(this->GetActor()->GetMoney() +
                               this->request_amount_);

    // increase opinion of actor (got money)
    this->GetActor()->AddRelationship(std::make_unique<RelationshipModifier>(
        this->GetTarget(), gamestate->Now(), gamestate->Now() + 10,
        this->request_amount_));
    // decrease opinion of target (gave money)
    this->GetTarget()->AddRelationship(std::make_unique<RelationshipModifier>(
        this->GetActor(), gamestate->Now(), gamestate->Now() + 10,
        -1.0 * this->request_amount_));

    SetReward(request_amount_);
    gamestate->GetLogger()->Log(DEBUG, "request by %d to %d of %f\n", this->GetActor()->GetId(),
           this->GetTarget()->GetId(), this->request_amount_);
  } else {
    // on failure:
    // decrease opinion of actor (refused request)
    this->GetActor()->AddRelationship(std::make_unique<RelationshipModifier>(
        this->GetTarget(), gamestate->Now(), gamestate->Now() + 10,
        this->request_amount_));
    SetReward(0.0);
    gamestate->GetLogger()->Log(DEBUG, "request_failed by %d to %d of %f\n", this->GetActor()->GetId(),
           this->GetTarget()->GetId(), this->request_amount_);
  }

  assert(GetTarget()->GetMoney() >= 0.0);*/
}

AskSuccessAction::AskSuccessAction(Character* actor, double score,
                                   std::vector<double> features,
                                   Character* target, AskAction* source_action)
    : Action(__FUNCTION__, actor, target, score, features),
      source_action_(source_action) {}

bool AskSuccessAction::IsValid(const CVC* gamestate) {
  return GetActor()->GetMoney() >= source_action_->GetRequestAmount();
}

void AskSuccessAction::TakeEffect(CVC* gamestate) {
  double request_amount = source_action_->GetRequestAmount();
  // on success:
  // transfer request_amount_ from actor to target
  GetActor()->SetMoney(GetActor()->GetMoney() - request_amount);
  GetTarget()->SetMoney(GetTarget()->GetMoney() + request_amount);

  // increase opinion of target (got money)
  GetTarget()->AddRelationship(std::make_unique<RelationshipModifier>(
      GetActor(), gamestate->Now(), gamestate->Now() + 10, request_amount));
  // decrease opinion of actor (gave money)
  GetActor()->AddRelationship(std::make_unique<RelationshipModifier>(
      GetTarget(), gamestate->Now(), gamestate->Now() + 10,
      -1.0 * request_amount));

  SetReward(-request_amount);
  source_action_->SetReward(request_amount);
}

bool StealAction::IsValid(const CVC* gamestate) {
  return GetTarget()->GetMoney() > steal_amount_;
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
    : Action(__FUNCTION__, actor, target, score, features),
      gift_amount_(gift_amount) {}

bool GiveAction::IsValid(const CVC* gamestate) {
  return this->GetActor()->GetMoney() > gift_amount_;
}

void GiveAction::TakeEffect(CVC* gamestate) {
  // transfer gift_amount_ from actor to target
  this->GetActor()->SetMoney(this->GetActor()->GetMoney() - this->gift_amount_);
  this->GetTarget()->SetMoney(this->GetTarget()->GetMoney() +
                              this->gift_amount_);

  // increase opinion of target (got money)
  double opinion_buff = this->gift_amount_;
  this->GetTarget()->AddRelationship(std::make_unique<RelationshipModifier>(
      this->GetActor(), gamestate->Now(), gamestate->Now() + 10, opinion_buff));

  SetReward(-gift_amount_);
  gamestate->GetLogger()->Log(
      DEBUG, "gift by %d to %d of %f (increase opinion by %f)\n",
      this->GetActor()->GetId(), this->GetTarget()->GetId(), this->gift_amount_,
      opinion_buff);

  assert(GetActor()->GetMoney() >= 0.0);
}


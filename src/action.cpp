#include "action.h"
#include "core.h"

Action::Action(Character* actor, double score) {
    this->actor_ = actor;
    this->score_ = score;
}

Action::~Action() {
}

Character* Action::GetActor() {
    return this->actor_;
}

void Action::SetActor(Character* actor) {
    this->actor_ = actor;
}

double Action::GetScore() {
    return this->score_;
}

void Action::SetScore(double score) {
    this->score_ = score;
}

TrivialAction::TrivialAction(Character* actor, double score) : Action(actor, score) {
}

bool TrivialAction::IsValid(const CVC& gamestate) {
    return true;
}

void TrivialAction::TakeEffect(CVC& gamestate) {
    printf("stuff by %d\n", this->GetActor()->GetId());
}

bool AskAction::IsValid(const CVC& gamestate) {
    return this->target_->GetMoney() > request_amount_;
}

void AskAction::TakeEffect(CVC& gamestate) {
    //TODO: ask action
    //check to see if the target will accept
    //opinion < 0 => no, otherwise some distribution improves with opinion

    //on success:
    //transfer request_amount_ from target to actor
    //increase opinion of actor (got money)
    //decrease opinion of target (gave money)

    //on failure:
    //decrease opinion of actor (refused request)
}

bool StealAction::IsValid(const CVC& gamestate) {
    return this->target_->GetMoney() > steal_amount_;
}

void StealAction::TakeEffect(CVC& gamestate) {
    //TODO: steal action
    //some distribution improves with opinion

    //on success:
    //transfer steal_amount_ from target to actor
    //decrease opinion of target by a lot (got money stolen)

    //on failure:
    //decrease opinion of actor by  a little bit (tried to get money stolen)
}

bool GiveAction::IsValid(const CVC& gamestate) {
    return this->GetActor()->GetMoney() > gift_amount_;
}

void GiveAction::TakeEffect(CVC& gamestate) {
    //transfer gift_amount_ from actor to target
    this->GetActor()->SetMoney(this->GetActor()->GetMoney() - gift_amount_);
    this->target_->SetMoney(this->target_->GetMoney() + gift_amount_);

    //increase opinion of target (got money)
    this->target_->AddRelationship(std::make_unique<RelationshipModifier>(this->GetActor(), gamestate.Now(), gamestate.Now()+100, gift_amount_));
}


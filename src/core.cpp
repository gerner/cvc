#include <vector>
#include <memory>

#include "core.h"

double Character::GetMoney() {
    return this->money_;
}

void Character::SetMoney(double money) {
    this->money_ = money;
}


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
    printf("stuff\n");
}

std::vector<std::unique_ptr<Action>> DecisionEngine::EnumerateActions(const CVC& cvc, Character* character) {
    std::vector<std::unique_ptr<Action>> ret;

    ret.push_back(std::make_unique<TrivialAction>(character, 1.0));

    return ret;
}


CVC::CVC(std::unique_ptr<DecisionEngine> decision_engine, std::vector<std::unique_ptr<Character>> characters) {
    this->decision_engine_ = std::move(decision_engine);
    this->characters_ = std::move(characters);
}

void CVC::GameLoop() {
    //TODO: don't just loop for some random hardcoded number of iterations
    for (int i = 0; i < 10; i++) {
        //1. evaluate queued actions
        this->EvaluateQueuedActions();
        //2. choose actions for characters
        this->ChooseActions();

        //TODO: expose state somehow to keep track of how stuff is going
        //  maybe also use the record of the state as training
        printf(".");
    }
}

void CVC::EvaluateQueuedActions() {
    //go through list of all the queued actions
    for(auto& action: this->queued_actions_) {
        //ensure the action is still valid in the current state
        if(!action->IsValid(*this)) {
            //if it's not valid any more, just skip it
            continue;
        }

        //let the action's effect play out
        //  this includes any character interaction
        action->TakeEffect(*this);
    }

    //lastly, clear all the actions
    this->queued_actions_.clear();
}

void CVC::ChooseActions() {
    //go through list of all characters
    for(auto& character: this->characters_) {
        //run the decision making loop for each:
        double stop_prob = 0.7;
        while(true) {
            //check to see if we should stop choosing actions
            if(rand() / (RAND_MAX + 1.) < stop_prob) {
                break;
            }

            //enumerate and score actions
            std::vector<std::unique_ptr<Action>> actions = this->decision_engine_->EnumerateActions(*this, character.get());

            //choose one
            double choice = rand() / (RAND_MAX + 1.);
            double sum_score = 0;
            //CVC is responsible for maintaining the lifecycle of the action
            for(auto& action: actions) {
                sum_score += action->GetScore();
                if(choice < sum_score) {
                    //keep this one action
                    this->queued_actions_.push_back(std::move(action));
                    //rest of the actions will go out of scope and 
                    break;
                }
            }

            //update stop_prob
            stop_prob = 1.0 - (1.0 - stop_prob) * 0.2;
        }
    }
}

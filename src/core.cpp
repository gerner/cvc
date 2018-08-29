#include <vector>
#include <memory>
#include <random>

#include "core.h"
#include "action.h"

RelationshipModifier::RelationshipModifier(Character* target, int start_date, int end_date, double opinion_modifier_) {
    this->target_ = target;
    this->start_date_ = start_date;
    this->end_date_ = end_date;
    this->opinion_modifier_ = opinion_modifier_;
}

Character::Character(int id, double money) {
    this->id_ = id;
    this->money_ = money;
}

int Character::GetId() const {
    return this->id_;
}

double Character::GetMoney() const {
    return this->money_;
}

void Character::SetMoney(double money) {
    this->money_ = money;
}

void Character::ExpireRelationships(int now) {
    for (auto it = this->relationships_.begin(); it != this->relationships_.end();){
        RelationshipModifier* relationship = it->get();
        if(now >= relationship->end_date_) {
            this->relationships_.erase(it++);
        } else {
            it++;
        }
    }
}

void Character::AddRelationship(std::unique_ptr<RelationshipModifier> relationship) {
    this->relationships_.push_back(std::move(relationship));
}

std::vector<std::unique_ptr<Action>> DecisionEngine::EnumerateActions(const CVC& cvc, Character* character) {
    std::vector<std::unique_ptr<Action>> ret;

    ret.push_back(std::make_unique<TrivialAction>(character, 1.0));

    return ret;
}


CVC::CVC(
        std::unique_ptr<DecisionEngine> decision_engine,
        std::vector<std::unique_ptr<Character>> characters,
        std::mt19937 random_generator) {
    this->ticks = 0;
    this->random_generator_ = random_generator;
    this->decision_engine_ = std::move(decision_engine);
    this->characters_ = std::move(characters);
}

void CVC::GameLoop() {
    //TODO: don't just loop for some random hardcoded number of iterations
    for (; ticks < 100; ticks++) {
        //1. evaluate queued actions
        this->EvaluateQueuedActions();
        //2. choose actions for characters
        this->ChooseActions();

        //TODO: expose state somehow to keep track of how stuff is going
        //  maybe also use the record of the state as training
        printf(".");
    }
}

int CVC::Now() const {
    return ticks;
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
    std::uniform_real_distribution<> dist(0.0, 1.0);
    //go through list of all characters
    for(auto& character: this->characters_) {
        //run the decision making loop for each:
        double stop_prob = 0.7;
        while(true) {
            //check to see if we should stop choosing actions
            if(dist(random_generator_) < stop_prob) {
                break;
            }

            //enumerate and score actions
            std::vector<std::unique_ptr<Action>> actions = this->decision_engine_->EnumerateActions(*this, character.get());

            //choose one
            double choice = dist(random_generator_);
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

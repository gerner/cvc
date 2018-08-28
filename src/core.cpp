#include <vector>
#include <memory>

#include "core.h"

double Character::GetMoney() {
    return this->money_;
}

void Character::SetMoney(double money) {
    this->money_ = money;
}


Character* Action::GetActor() {
    return this->actor_;
}

double Action::GetScore() {
    return this->score_;
}


void CVC::GameLoop() {
    //TODO: don't just loop for some random hardcoded number of iterations
    for (int i = 0; i < 1000; i++) {
        //1. evaluate queued actions
        this->EvaluateQueuedActions();
        //2. choose actions for characters
        this->ChooseActions();

        //TODO: expose state somehow to keep track of how stuff is going
    }
}

void CVC::EvaluateQueuedActions() {
    //go through list of all the queued actions
    for(auto const& action: this->queued_actions_) {
        //ensure the action is still valid in the current state
        if(!action->IsValid(this)) {
            //if it's not valid any more, just skip it
            continue;
        }

        //let the action's effect play out
        //  this includes any character interaction
        action->TakeEffect(this);
    }

    //lastly, clear all the actions
    this->queued_actions_.clear();
}

void CVC::ChooseActions() {
    //go through list of all characters
    for(auto const& character: this->characters_) {
        //run the decision making loop for each:
        double stop_prob = 0.7;
        while(true) {
            //check to see if we should stop choosing actions
            if(rand() / (RAND_MAX + 1.) < stop_prob) {
                break;
            }

            //enumerate and score actions
            //choose one
            double choice = rand() / (RAND_MAX + 1.);
            //CVC is responsible for maintaining the lifecycle of the action

            //update stop_prob
            stop_prob = 1.0 - (1.0 - stop_prob) * 0.2;
        }
    }
}

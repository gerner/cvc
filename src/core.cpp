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
            printf("expiring %d -> %d (%f)\n", this->GetId(), relationship->target_->GetId(), relationship->opinion_modifier_);
            this->relationships_.erase(it++);
        } else {
            it++;
        }
    }
}

void Character::AddRelationship(std::unique_ptr<RelationshipModifier> relationship) {
    this->relationships_.push_back(std::move(relationship));
}

double Character::GetOpinionOf(Character *target) const {
    double opinion = 0.0;
    //TODO: non-relationship-modified opinion traits

    //TODO: rethink how we're organizing these relationships to make this faster
    for(const auto& r : this->relationships_) {
        if(r->target_ == target) {
            opinion += r->opinion_modifier_;
        }
    }

    return opinion;
}

std::vector<std::unique_ptr<Action>> DecisionEngine::EnumerateActions(const CVC& cvc, Character* character) {
    std::vector<std::unique_ptr<Action>> ret;

    //GiveAction
    Character* best_target = NULL;
    double worst_opinion = std::numeric_limits<double>::max();
    if(character->GetMoney() > 10.0) {
        for(Character* target : cvc.GetCharacters()) {
            //skip self
            if(character == target) {
                continue;
            }

            //pick the character that likes us the least
            double opinion = target->GetOpinionOf(character);
            if(opinion < worst_opinion) {
                worst_opinion = opinion;
                best_target = target;
            }
        }
        if(best_target) {
            ret.push_back(std::make_unique<GiveAction>(character, 0.4, best_target, character->GetMoney() * 0.1));
        }
    }

    //AskAction
    best_target = NULL;
    double best_opinion = 0.0;
    for(Character* target : cvc.GetCharacters()) {
        //skip self
        if(character == target) {
            continue;
        }

        if(target->GetMoney() <= 10.0) {
            continue;
        }

        //pick the character that likes us the least
        double opinion = target->GetOpinionOf(character);
        if(opinion > best_opinion) {
            best_opinion = opinion;
            best_target = target;
        }
    }
    if(best_target) {
        ret.push_back(std::make_unique<AskAction>(character, 0.4, best_target, 10.0));
    }

    ret.push_back(std::make_unique<TrivialAction>(character, 0.2));

    return ret;
}


CVC::CVC(
        std::unique_ptr<DecisionEngine> decision_engine,
        std::vector<std::unique_ptr<Character>> characters,
        std::mt19937 random_generator) {
    this->ticks_ = 0;
    this->random_generator_ = random_generator;
    this->decision_engine_ = std::move(decision_engine);
    this->characters_ = std::move(characters);
}

std::vector<Character *> CVC::GetCharacters() const {
    std::vector<Character *> ret;//(this->characters_.size());
    for(auto& character : this->characters_) {
        ret.push_back(character.get());
    }
    return ret;
}

void CVC::GameLoop() {
    //TODO: don't just loop for some random hardcoded number of iterations
    for (; this->ticks_ < 1000; this->ticks_++) {
        //0. expire relationships
        this->ExpireRelationships();
        //1. evaluate queued actions
        this->EvaluateQueuedActions();
        //2. choose actions for characters
        this->ChooseActions();

        //TODO: expose state somehow to keep track of how stuff is going
        //  maybe also use the record of the state as training
        this->PrintState();
    }
}

void CVC::PrintState() const {
    //TODO: probably don't want to always spit to stdout

    printf("tick %d:\n", this->ticks_);
    for(const auto& character: this->characters_) {
        printf("%d	%f	", character->GetId(), character->GetMoney());
        for(const auto& target: this->characters_) {
            printf("%f	", character->GetOpinionOf(target.get()));
        }
        printf("\n");
    }
}

int CVC::Now() const {
    return this->ticks_;
}

std::mt19937& CVC::GetRandomGenerator() {
    return this->random_generator_;
}

void CVC::ExpireRelationships() {
    for(const auto& character: this->characters_) {
        character->ExpireRelationships(this->Now());
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

            //we assume if there's no actions there will not be any this tick
            if(actions.empty()) {
                break;
            }

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

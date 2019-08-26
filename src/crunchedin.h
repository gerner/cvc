#ifndef CRUNCHEDIN_H_
#define CRUNCHEDIN_H_

#include <limits>
#include <vector>

#include "core.h"

class Organization {
 public:

 private:
  Character* ceo_;
  std::vector<Character*> current_staff_;
  int start_tick_;
  int end_tick_ = std::numeric_limits<int>::max();

  //TODO: contributions is a placeholder for the total score for this org
  //    in the future this ought to be a more complicated function of products,
  //    customers, sales, etc.
  double contributions_ = 0.0;
};

struct Role {
  Character* character_;
  Organization* org_;
  int start_tick_;
  int end_tick_ = std::numeric_limits<int>::max();

  //TODO: contribution is a placeholder for contributions by this character
  //    in the future this ought to be a more complicated function of stuff
  //    this character has done
  double contribution_ = 0.0;

  //represents how "good" the org was when this character had this role
  //together represents how much the org improved during the character's tenure
  //useful to understand the character's role in that improvement, e.g.
  //contribution_ / (contributions_at_end_ - contributions_at_start_)
  //the value of corp contributions at start/end from this role
  //end is meaningless unless end_tick_ <= CVC.Now()
  double contributions_at_start_;
  double contributions_at_end_;
};

//records career history, but also the crunchedin representation of a character
class CurriculumVitae {
 public:
  double TotalContribution() const {
    double score = 0.0;
    for(const Role& role: roles_) {
      score += role.contribution_;
    }
    return score;
  }
 private:
  std::vector<Role> roles_;
};

#endif

# Proposal and Response

Many actions in CVC consist of putting a proposal to an agent and letting that
agent decide how to respond. For instance, a hiring manager might propose to
hire a job candidate. That candidate can respond by either accepting or decline
the offer.

Note, these responses are not meant to model ongoing relationships between
characters. For instance, one character might take some action which might be
reasonably considered as antagonistic towards another character. This
proposal/response mechanism should not be used to play out a larger scale
response to such an antagonistic response.

## Design

Every game tick consists of:
* world maintenance (e.g. expiring relationships)
* evaluating actions
* choosing new actions

Proposals should be explicitly responded to (vs, e.g. implicitly being declined
because the positive response action is not selected). And these responses
should use a similar mechanism to choosing actions (e.g. evaluating state,
learning based on outcomes).

To support this, actions, and corresponding action factories, will not only be
able to enumerate actions (creating them as options the agent might choose),
but they should also have logic to respond to proposal actions.

When proposal actions are evaluated, instead of immediately taking effect, the
target of the proposal (to whom the action is being proposed) will have an
opportunity to respond (accepting or declining the proposal). According to that
response the action will play out or not.

This means that when evaluating an action, we need not only the target
character (holding and manipulating game state for that character), but we also
need access to the decision making capacity for that character, i.e. the agent
representing that character.

TODO: impact on relationship (e.g. accepting/declining a proposal has a
positive/negative relationship modifier? maybe not, and relationship modifier
is built into the action).

## Learning

Responding to actions is an experience for the agent. In this case the agent
has exactly two options: accepting the proposal or rejecting it and it's making
a choice between those two options. The subsequent action is chosen separately,
but none-the-less the agent has an experience consisting of an action (accept
or reject) and a reward (possibly 0 if the proposal is rejected), as well as an
updated game state.

So we can learn from this experience. We'll keep track of responses to actions
and create corresponding experiences for the recipient of the proposal.

In this way, every action could have a pair of learning experiences: one for
the actor and one for the target. Similarly, there's a pair of rewards, one for
each actor/target.

for learning actions consist of a score (the estimate of Q(s, a) under this
action), reward for taking this action, and some representation of (or perhaps
an approximation of) s.

deciding a response consists of estimating the function Q(s, a) for two
possible actions: accept or reject (let's call this `a_a` and `a_r`)

`Q(s, a_a)` and `Q(s, a_r)`

we take one of those actions and what plays out is:
* we receive some reward r corresponding to accepting or rejecting the proposal
  (e.g. reject, no immediate reward, we accept, get some immediate reward from
the proposal)
* we get some new state s' corresponding to having accepted/rejected the
  proposal (e.g. we have some new relationship with the proposer)
* we've chosen some new action based on the new state, a'

with this information we can apply learning based on the (perhaps estimated)
error:

```
d = r + g * Q(s', a') + Q(s, a)
```

to accomplish this we need to:
* estimate `Q(s, a_a)` and `Q(s, a_r)`
* know the reward for `a_a` and `a_r`
* know the updated estimate having taken one of those actions and choosing some
  new action

## Implementation

I'd like to model this as just more actions that get taken. The agent faces a
couple of choices: accept or decline. It scores those options and takes one.
This updates the game state, and finally (just like normal) it choose some new
actions.

so we compute a score for either accepting or rejecting the proposal: `Q(s,
a_a)` and `Q(s, a_r)`. We pick one according to those scores. It plays out
(perhaps immediately) with some immediate reward. This updates the game state.
According to that new state we pick a new action with a new score. This gives
us an experience to learn from.

So a proposal from one agent presents another with some possible actions,
perhaps there could be several actions (e.g. politely decline, accept, become
enraged, etc.). This should look a lot like choose an action given the current
game state. But there's an important piece of extra context: the particular
action the agent is responding to. So we need some special kind of
ActionFactory that can enumerate some actions (one of which we'll take, just
like normal).

So we now have a pair of actions:
* the original proposal
* the response to the proposal

both of these play out. The result will be some reward for each agent due to
each action. But the reward depends the response.

We have two experiences from this interaction. One for the proposing agent, of
the form:
* s = original state
* a = proposal action
* r = reward from the proposal/response playing out
* s' = updated state following proposal/response
* a' = new action chosen based on s'

Where s' is the result of all actions playing out and a' is chosen during the
normal ChooseActions phase of GameLoop.

We also have an experience for the proposee, of the form:
* s_hat = state at the time the proposal is considered
* a_response = response to the proposal a
* r_response = reward from responding to the propsal accoridng to a_response
* s' = updated state followin proposal/response
* a' = new action chosen based on s'

Where s' is the result of all actions playing out (including the response) and
all other actions playing out. a' is chosen during the normal ChooseActions
phase of GameLoop.

A few updates to the exising ActionFactory model (and learning)
* need a way to find the appropriate ActionFactory to respond to an action
* that ActionFactory needs to accept both CVC and the proposal as input state
* The reward for the proposal depends on the response (needs to be set by
  response)
* This interaction creates a new experience for the responder to learn from


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


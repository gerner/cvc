# Proof of Concept Design

## Objective

Explore basic game concepts:
* many characters all taking action independently
* relationships/opinions between characters
* some kind of simple resource management
* basic AI decision making
* actions AI (and player) can take
* game cadence (clock ticks) and how actions interact with that cadence

## Plan

We'll explore those basic game concepts in the context of a simulation of many
agents, each representing the decision making of one character.

### Cadence

A clock will operate with ticks. On every tick all agents will have an
opportunity to take one or more actions. Every tick actions taken will play
out. We'll need to serialize playing out actions and agent decision making.
Ideally, actions are playing out simultaneously and agents are making decisions
simultaneously. Also, we should not end up in some inconsistent state where an
action was chosen at one time and then executed at some later time when that
choice is no longer valid.


A tick will include several phases:
* Execute queued actions, revalidating the actions an letting their effect play out.
* Notifications (requiring agent interaction) are revalidated and the receiving agent may respond. The resulting effect plays out.
* Agents choose zero or more actions to take

### Resources

Every character has money. The objective is to maximize money.

### Actions

Actions have a set of requirements (preconditions) to be "valid". For example,
you can't choose to spend money you don't have. The actions also have some
effect that will play out when the action is taken. Every action's effect (or
all possible effects) must leave the game in a consistent state if the action
is valid. Actions might also be parameterized to target a particular character.

PoC Actions:
* Ask Y character for money: Y has to have the money, and will, with some probability, based on opinion give the character some money. success reduces opinion of giver, failure reduces opinion of recipient
* Steal money from Y character: Y has to have the money. Has some probaility of success (based on opinion). Has some probaility of detection (based on opinion).
* Give Y character some money: Character has to have the money, gives Y money and increases Y's opinion (based on Y's current money and opinion)

### Choosing Actions

Agents choose an action from a vector of valid actions. Each element of the
vector has a score representing the agent's likelihood to choose that action.
Stopping is another choice. The action scores plus the probability of stopping
forms a probability distribution. The process of scoring valid actions and
choosing one repeats until the agent chooses to stop.

The decision making (enumerating and scoring choices) will be abstracted by
some decision engine. But, for example, we might choose to stop with some
probability that increases with each choice. Other choices are scored against
the remaining probability.

Valid actions must be enumerated and scored. Because valid actions can be
parameterized (with a target), we might simplify the space by choosing one
target for each action.


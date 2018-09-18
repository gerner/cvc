# Decision Engine

## Arch

CVC class is just for holding instantaneous state of the game (characters,
relationships, etc.) All the logic for choosing actions (based on that state),
and manipulating that state will live in the DecisionEngine.

### Action

Action is a baseclass encapsulating an action which will (if successfully
carried out) update the state of the game. Actions consist of some state:

* Actor
* Score
* Action specific state

Actions also hold some logic:

* IsValid(CVC): true iff this action is valid in the context of some game state
* TakeEffect(CVC): (tries, depending on potentially some probabalistic state)
  to take effect, updating the state.

### DecisionEngine

The DecisionEngine evaluates actions across all the characters in the game. It
uses a set of ActionFactory instances to, for each character, enumerate
possible actions for the current game state, scores those actions. It then
selects up to one action for each character per tick.

The DecisionEngine then keeps track of all actions for the tick which are
executed at the beginning of the following tick, possibly updating the game
state. New actions are then selected.


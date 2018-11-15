# Summarizing Game State

This covers features for modelling action decision making, inspired by
reasonably summarizing game state for user view. That is, a concise and
meaningful summary of the game state that a user would find useful would also
make reasonable features for model training.

## Global Game State

Features that do not vary with the character. These simply summarize the global
game state from an external, non-character-specific perspective.

* Average, stdev, median money
* Average, stdev, median opinion

## Per-player game state

* Player's money minus average and median global money
* Player's average/median opinion minus average/median opinion


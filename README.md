# John Conway's game of life
With modified rules.

Modified rules keep large enough (500x500+) field always alive, with new activity emerging here and there. Small field could die out completely but can't stuck in a still state.

Modified rules (on top of classical ones):
*1. If cell is alive for 100 cycles, it has to either die or reborn (age is reset to 1).
Chance of death is set to 1%.
*2. If cell is dead but for 100 cycles had at least one alive neighbour, then if it has
exactly 2 neighbours, it has 1% chance of becoming alive. Otherwise, counter is reset to 0.

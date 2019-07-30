# NLTH Poker Agent using Counterfactual Regret Minimization

* Trains strategies for Kuhn-, Leduc- and Texas Holdem Poker
* Supported Training methods:
  * External Sampling
  * Chance Sampling
  * Outcome Sampling

## Requirements
* Clang (For C++11 support)
* Boost Programmoptions 1.55+

## Installation
* Clone repositories and copy HandRanks.dat into cfrm:
```
$ git clone https://github.com/pandaant/cfrm.git
$ git clone https://github.com/christophschmalhofer/poker.git
$ cp poker/XPokerEval/XPokerEval.TwoPlusTwo/HandRanks.dat cfrm/handranks.dat
$ #rm -r poker # was only needed for handranks.dat
```
* Compile required libraries:
```shell
$ cd cfrm/lib
$ cd libpoker && make && cd ..
$ cd libecalc && make && cd ..
```
* Compile EHS tool and generate Expected Handstrength table (EHS.dat):
```shell
$ cd cfrm/tools/ehs_gen
# tweak values for number of threads and number of samples in src/gen_eval_table.cpp if neccessary
$ make
$ ./gen_eval_table #( may take some time! )
$ mv ehs.dat ../../ehs.dat
```
* Build agent binaries
```shell
$ cd cfrm 
$ make
```

## Usage
If the build process was successful 4 binaries have been created:

* ./cfrm is the main executable that trains a strategy.
* ./cluster-abs generates card abstractions based of different metrics ( explained below ).
* ./potential-abs generates a potential based card abstraction based on a precalculated cluster abstraction.
* ./player can be used to play the agent against itself or other agents ( uses UAPRG server )

* The scripts folder contains example scripts to generate abstractions and strategies for different games.

### Action Abstraction

A action abstraction discretizes the range of betting possibilities a player may choose from.
This abstraction can reduce the number of states drastically. 

* NullActionAbstraction 
* PotRelationAbstraction

### Card Abstraction

NLTH has a large number of chance events due to private-card dealings preflop and public-card
dealings post-flop. To reduce the number of outcomes that a chance event may result in, card
combinations that are similar in strength are combined into buckets. These buckets are then
used as new chance events in the abstract game.

#### NullCardAbstraction
#### BlindCardAbstraction
#### ClusterCardAbstractions
* SI

* EHS - Expected Hand Strength

  E[HS] or E[HS 2] metric groups hands into buckets.
  Although E[HS] is a good first estimator it can’t distinguish between hands that realize their
  expectation in different stages of the game.

* EMD - Earth Movers Distance

* OCHS - Opponent Cluster Hand Strength

  OCHS can only be applied to the River phase. The E[HS] value is the probability to win against a random
  distribution of opponent holdings. OCHS splits the opponents possible holdings into n buckets.
  Each index in the histogram then corresponds to the probability of winning against hands that
  are in the bucket with the same index.

  OCHS can be used cluster river hands using a distribution-aware approach. It has been shown
  that OCHS based river abstractions outperformed expectation based abstractions.

* Mixed Abstractions

  N stands for no abstraction, S for E[HS ], E for EMD clustering over histograms, O for OCHS and P for
  the potential aware abstraction. The Ordering of the letters determines which abstraction is used in which phase of the game (preflop, flop, turn, river).

  * MIXED_NEEO
  * MIXED_NEES
  * MIXED_NSSS
  * MIXED_NOOO

### Sampling Schemes

* ChanceSampling

  Chance sampled CFR randomly selects private and public chance events. The portion of the
  game-tree that is reachable through the sampled chance events is traversed recursively. A vector
  of probabilities is passed from the root down the tree containing the probability of each players
  contribution to reach the current node in the tree. In all reached terminal nodes the utility can
  be calculated in O(1).

* ExternalSampling

  External sampling only samples factors that are external to the player, namely chance nodes
  and opponents actions. It has been proven that external sampling only requires a constant
  factor more iterations in contrast to vanilla CFR. It still archives an asymptotic improvement in
  equilibrium computation time because of an order reduction in cost per iteration. External
  sampling performs a post-order depth-first traversal during an iteration.

* OutcomeSampling

  Outcome sampling only visits one terminal history in each iteration and updates the
  regrets in information sets visited along the path traversed.

### Action Translation 

* PseudoHarmonicMapping

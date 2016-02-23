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
### Card Abstraction
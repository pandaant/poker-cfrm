# Poker Agent using Counterfactual Regret Minimization

## Requirements
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
* script folder contains example scripts to generate abstractions and strategies


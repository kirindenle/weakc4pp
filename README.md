2swap's repo https://github.com/2swap/WeakC4
validate.cpp is almost 1-to-1 python implementation from solution/validate_solution.py for leafs verification
validate-v2.cpp is a little bit optimized and contains code for new branches generation
The idea is to reuse steady states so for each steady state we collect all branches that it solves, then we select some of the states that will still cover all the branches. I use greedy algorithm by simply choosing states that have max additional coverage, so as far as I know it does not guarantee an optimal selection.


#ifndef CPP_BRUE_FORCE_COMPLETE_SOLVER_H
#define CPP_BRUE_FORCE_COMPLETE_SOLVER_H

#include "complete_solver.h"

class BfLiftingSolver: public LiftingSolver {
    int step_size_1 = 1;
    int step_size_2 = 100;
    Grammar* lifting_grammar;
    std::vector<Grammar*> combinator_grammar_list;
    Grammar* buildLiftingGrammar();
    Grammar* buildCombinatorGrammar(int size);
public:
    BfLiftingSolver(const std::vector<Task*>& _task_list): LiftingSolver(_task_list), lifting_grammar(buildLiftingGrammar()) {
    }
    virtual void synthesis();
};

#endif //CPP_BRUE_FORCE_COMPLETE_SOLVER_H

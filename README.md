The supplementary material for POPL23 submission "Divide and Conquer Divide-and-Conquer -- Black-Box Synthesis for Efficient Algorithms on Data Structures"

#### Source Code

Our implementation of *AutoLifter* is stored in folder `cpp`, which is organized in the following way.

```
├── basic                             	// Basic libraries used in the implementation.
├── exp                             	// Tasks used in the evaluation.
│   ├── benchmark.json                  // A summary of all tasks.
│   ├── divide_and_conquer.cpp          // The implementation of tasks in for parallelization.
│   ├── longest_segment.cpp             // The implementation of tasks in for LSP.
│   ├── lazy_propagation.cpp            // The implementation of tasks in for RANGE.
├── solver                             	
│   ├── complete_solver                 // The main solver for lifting problems.
│   ├── enumerate_based_solver          // The implementation of observational covering
│   ├── task                            // The definition of the instances of lifting problems
│   ├── brute_force_complete_solver     // The implementation of baseline Enum.
│   ├── hfta_solver                     // The implementation of baseline Relish.
├── polygen                             // The external solver for operator tau. 
```

#### Evaluation Results

All evaluation results are stored in folder `exp`, which is organized in the following way.

```
├── result_cache                        // Summaries of results for all involved solvers.
├── oup                                 // Detailed outputs for each solver.
│   ├── AutoLifter                      // Taking AutoLifter as an example.
│   │   ├── d                           // Outputs of AutoLifter on tasks for parallelization.
│   │   ├── l                           // Outputs of AutoLifter on other tasks.
```


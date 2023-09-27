# TabuForJSSP
A tabu search algorithm and a memetic algorithm for solving the job shop scheduling problem, 
written by Laurenz, inspired by the work of Zhang et al. [1], E. Balas and A. Vazacopoulos [2] and [3]. 
### In this document:
- JSSP : problem description
- About : background of the project
- Guide : dev guide
- Links : link collection for reference and context

## JSSP
The Job Shop Scheduling Problem (JSSP) is a classical NP-hard problem. Scheduling is the allocation of 
resources to tasks (or operations) in a time based environment. The JSSP is the most well known problem 
of the family of scheduling problems. The resources are called machines in this problem, time is expressed as 
integer number and the operations have job dependencies.  A problem description in short: 

Given:
- a set of jobs, consisting of a sequence of operations (duration, machine)

Wanted:
- a minimal schedule - a starting time has to be assigned to each operation

Constraints:
- each operation can be started, if the predecessing operation in its job is done processing 
(starting time + duration = end time) 
- each machine can only process one operation at a time

For detail check: [TODO]

## About
I am a german computer scientist and wrote these algorithms in my bachelor thesis and improved and documented
them later on. Both algorithms are strongly inspired by the work of Zhang et al. [1], E. Balas and 
A. Vazacopoulos [2] and [3]. My thesis is not and won't become publicly available but in short; it was about
scanning the landscape of the JSSP for good solutions and comparing a selection of them. Because I did not 
find any publicly available and well documented metaheuristics, I decided to implement some myself, because
they seemed to have superior performance to heuristic and exact approaches, when good quality solutions are
required. Therefore, I reviewed the results of many more recent papers in this area and came to the conclusion,
that those two algorithms might be suitable and could be one of the best in the desired metrics. To give you
the chance to compare your implementation of a JSSP algorithm against some well documented (and I think good)
competitors with some scientific backing.
My results showed, that Google OR Tools is pretty good at finding optimal solutions, but the Tabu 
Search was faster in finding solutions with 5%-optimality gap. Also, on some instances it could outperform 
the OR Tools formulation by far. The Memetic algorithm performed okay, so I share it anyway, but more as an 
educational example. I will share some results, an implemented Google OR Tools CP-SAT formulation in Python,
and a Python package in a repository on this profile in the future. If you have questions or problems file me
an issue on this project and I will be glad to help you. If you have improvements, make a pull request. 
But please mind: **I do all of this in my free time.**

## Guide
Two chapter. Usage if you don't care about the implementation, implementation details if you want to understand
how it works. I recommend reading both before looking into the source code. More complex parts of the algorithm 
have references in the code to the paper which explains the part. 

### Usage
#### Tabu search
1. Initiate an instance of JSSPInstance (ji) with the path to the instance file as parameter (see jssp.cpp/.h)
2. Initiate a TabuSearch instance (ts) with the JSSPInstance as parameter (see ts.cpp/.h)
3. Create an initial solution with ji.generateRandomSolution()
4. Start optimization with ts.optimize() with the initial solution and a time limit (seconds) as parameter 
5. (4.) solves the instance and returns a Solution struct with the makespan, the solution as list of machine sequences
   and a list of the intermediate solution values with timestamps 

#### Memetic algorithm
1. Initiate an instance of JSSPInstance with the path to the instance file as parameter (see jssp.cpp/.h)
2. Initiate an instance of MemeticAlgorithm (mem) with the JSSPInstance as parameter
3. Start optimization with mem.optimize() with a time limit (seconds) as parameter
4. (3.) solves the instance and returns a Benchmark struct with the makespan, the solution as list of machine sequences
and a list of the intermediate solution values with timestamps

#### Additional hints
- Time limits are soft time limits. The algorithm will finish the current iteration and return afterwards if the time limit
is reached. This gets relevant especially on large instances. MemeticAlgorithm will also finish initialization, even if 
the time limit is reached earlier. 
- Both algorithms offer an iteration constraint optimization method. In this method the algorithm does not log.
- Tabu search can be started with any feasible starting solution.
- Memetic algorithm also offers the possibility to give starting solutions with optimizePopulation()
- Instances have to be in the format described in ./instances/README.md - the standard format from the OR-Library

### Implementation details
#### JSSPInstance (jssp.cpp/.h)
Utility class around the jssp instance. Provides structs Solution and BMResults for algorithm outputs. JSSPInstance 
can load and manage instances and also provides the random seed for the algorithms. All methods only work, if the instance
was correctly initialized.

#### Tabu search (ts.cpp/.h, tabu_list.h)
The Tabu search algorithm inspired by Zhang et al. [1], so if you are interested in more details on JSSP and tabu search 
on it, I strongly recommend reading their paper. Initialization takes an instance which provides the random seed,
utility methods and the data of the instance to be solved. TabuList is an additional class which provides an interface
to manage the tabu list, so prohibiting solution features, checking if a solution is prohibited and allowing solution
features again. The optimization starts by setting parameters. In the main loop each iteration it generates 
a neighbourhood so, a set of solutions which are similar to the current solution. These solutions have a change in the
longest path of the current solution, so the sequence of operations which prevents the schedule to be shorter. 
This is a good approach because the makespan can only be reduced by changing the longest path. Because we need to find 
the longest path, it is pretty intuitive to manage the solution as graph. Therefore, the algorithm manages the current
solution as a graph, called disjunctive graph. In it, each operation has an "edge" to the previous and next operation in 
its job and in its machine. This disjunctive graph is a network graph, so we are able to find the longest path using 
dynamic in a finite amount of time, unlike in a general graph, where the problem is NP-hard. Each operation is 
represented as a Node struct. The initial solution is generated at the start of the algorithm and updated each iteration
after the algorithm selected a neighbouring solution. The update reassembles the disjunctive graph so that the graph
represents the neighbour. The algorithm selects the neighbour which has the best approximated makespan and is not tabu.
If a neighbour has a better approximated makespan than the best makespan found, the makespan is calculated exact and if 
it is truly better, the neighbour is the next solution no matter if it is tabu (aspiration criterion). If all solutions
are tabu, a random is selected. 

Overview:
1. Create initial disjunctive graph
2. Set parameters
3. Each iteration:
   1. Find longest path
   2. Find neighbours by doing swaps on the longest path -> approximate makespans
   3. Update the current solution to the best neighbour, update disjunctive graph
   4. If new best solution -> check exact makespan, if truly better -> update bestSolution
4. Time / iteration limit reached: return best solution

The process of approximating the makespans of the neighbours is pretty complex and we need to calculate the "len to n"'s 
in beforehand. "len to n" is so the longest path to a node which has no job- and machine-successor for each node of 
the disjunctive graph. Calculating the "len to n"'s is the most expensive step in the algorithm, but is crucial to approximate
makespans, which would be the most expensive task otherwise (and even more expensive of course).  

#### Memetic algorithm
[TODO]
## Links
[TODO]

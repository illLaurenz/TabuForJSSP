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
But please mind; I do all of this in my free time.

## Guide
Two chapter. Usage if you don't care about the implementation, implementation details if you want to understand
how it works. I recommend reading both before looking into the source code.
### Usage

### Implementation details

## Links


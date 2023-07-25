## Instances

The Instances 
- abz_instances
- dmu_instances
- ft_instances
- la_instances
- orb_instances
- swv_instances
- ta_instances
- yn_instances

are from the OR-Library. I downloaded them at http://jobshop.jjvh.nl/explanation.php (18.02.2023) which corresponds to the paper "The Current state of bounds on benchmark instances of the job-shop scheduling problem" by Jelke J. van Hoorn. They can also be found (unformatted) in the famous OR-Library (http://people.brunel.ac.uk/~mastjjb/jeb/orlib/jobshopinfo.html) by J E Beasley.

They come in two formats. The one i use is the standart format which is present in each subfolder. The other format by Taillard can be found in each subfolder in Taillard_specification.

#### Standart format: 

In the first row the first represents the number of sequence and the second number the number of machines. Each following row represents one job. Therefor the numbers represent tuples as (machine number, time operation) so the order of the tuples represent the order in which the job has to be passed though the machines. The machine number starts at 0. The numbers are seperated by a tab symbol.


#### Taillard specification: 

"On the first line are two numbers, the first is the number of sequence and the second the number of machines. Following there are two matrice the first with a line for each job containing the processor times for each operation the second with the order for visiting the machines. The numbering of the machines starts at 1.

For example the same instance as above would be presented as:

1	3	
6	7	5
2	3	1" - Jelke van Hoorn, 2015. 

# Instances
I do not claim ownership for any of the instances in this project.

## In this README:
- Sources: where to get instances
- Instances structure: description
- References: links and references

## Sources
These and additional instances can be downloaded at [1] from the OR-Library by JE Beasly [2]. Jelke J van Hoorn did a 
comprehensive study on the bounds of many instances [3].

## Instance structure
All numbers in each row are seperated by a tab. The first number represents the number of jobs *n* and the second 
number the number of machines *m*. Each following row represents one job of the instance. The rows have to be read as 
table of tuples. So the first number is the machine required by the operation and the second the duration. The sequence
of the operations in their row represents their job dependency. So first operation has to be processed before the second
and so on. So in picture:

|        *n*         |         *m*         |                    |                     |        |
|:------------------:|:-------------------:|:------------------:|:-------------------:|:------:|
| Machine 1 of Job 1 | Duration 1 of Job 1 | Machine 2 of Job 1 | Duration 2 of Job 1 |  ...   |
| Machine 1 of Job 2 | Duration 1 of Job 2 | Machine 2 of Job 2 | Duration 2 of Job 2 |  ...   |
|        ...         |         ...         |        ...         |         ...         |  ...   |

## References
I do not take any responsibility for the content of the websites behind the links.

[1] (OR-Library) http://people.brunel.ac.uk/~mastjjb/jeb/orlib/jobshopinfo.html

[2] (OR-Library) "OR-Library: distributing test problems by electronic mail", Journal of the Operational Research Society 41(11) (1990) pp1069-1072.

[3] (Study by Jelke J van Hoorn) http://jobshop.jjvh.nl/ 

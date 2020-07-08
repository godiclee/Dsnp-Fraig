# Functionally Reduced And-Inverter Graph (FRAIG)
This is the final project of Data Structure and Programming (DSNP) in National Taiwan University.

This program provides the following functionalities:
1.  Parse the circuit description file in the AIGER format
2.  Sweep out the gates that cannot be reached from primary outputs. After this operation, all the gates that are          originally “defined-but-not-used” will be deleted.
3.  Perform trivial circuit optimizations (i.e. constant propagation)
4.  Perform structural hash to merge the structurally equivalent signals
5.  Simulate boolean logic to group potentially equivalent gates into functionally equivalent candidate (FEC) pair.
6.  Use a boolean satisfiability solver to formally prove or disprove FEC pair and merge equivalent gates.

For Implementation Details, please refer to REPORT.pdf (written in English)

For specification and detailed instructions, please refer to spec.pdf (from the course)

My program got the first place among ~170 students in the class, and passed all test cases in ./tests.fraig.
However, the program may still have some hidden bugs, and there is still a lot of room for improvement in terms of time and space complexity.  

Special thanks to Prof. Chung-Yang (Ric) Huang，lecturer of the course.

## How to Execute
Type make clean / make to compile the program. Execute ./fraig to run the program.

The program is tested under ubuntu 18.04 (gcc/g++ 7.5)

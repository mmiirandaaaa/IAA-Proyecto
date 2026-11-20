# Island Model Genetic Algorithm for the MDMKP

Genetic algorithm with two islands for the Multi-Demand Multidimensional Knapsack Problem. Island F searches for feasibility and ignores profit, island Q searches for profit and penalizes infeasibility. Every k generations the islands exchange their best individuals. The best feasible solution seen on either island is kept and returned at the end.

# Compile

```
make
```

It needs g++ (tested with g++ 13 on Ubuntu). To remove the compiled files:

```
make clean
```

# Run

```
./mdmkp [parameters] < <instance file>
```

The instance file goes in through standard input. All the parameters are optional. Each one is written as a flag followed by its value, in any order. The ones you do not write keep their default.

```
--q          1, 2 or 3. Number of demand constraints q = 1, m/2 or m    default 1
--coefs      mixed or positive. Sign of the profit coefficients          default mixed
--instancia  number of the instance inside the file (starts at 1)        default 1
--seed       seed of the random number generator                         default 1
--s          population size s of each island                            default 100
--pc         crossover probability                                       default 0.9
--pm         mutation probability of each gene                           default 0.01
--elite      number of elite individuals e                               default 2
--k          migrate every k generations                                 default 10
--mu         number of migrants                                          default 2
--t          target fraction of feasible individuals, island F           default 0.5
--gamma      adaptation rate of the weights, island F                    default 0.05
--gr         generations without improvement before restart              default 50
--gmax       maximum number of generations                               default 1000
--tmax       maximum time in seconds                                     default 10
--csv        name of a CSV file to write the evolution to                default none
```

The algorithm stops when it reaches gmax generations or tmax seconds, whichever comes first. The defaults are only starting values, they are not tuned.

# Example

```
./mdmkp --q 1 --coefs mixed --instancia 1 --seed 42 --s 100 --pc 0.9 --pm 0.01 --gmax 2000 --tmax 30 < instancias/mdmkp_ct1.txt
```

This solves instance 1 of the file with q = 1 and mixed profits, seed 42, islands of 100 individuals. Running just `./mdmkp < instancias/mdmkp_ct1.txt` uses all the defaults.

# Output

The program prints the instance and then the best solution found: the bit vector with its feasibility zone (KFDF means feasible), its profit, the number of generations and the time. If no feasible solution was found, it prints the individual with the smallest violation and the line "No se encontró solución factible".

# CSV

If you give a file name with --csv, the program writes one row per generation, plus a row for generation 0 (the initial populations). Without the flag no file is written.

```
generation,profit,feasible,best_F,best_Q
```

The columns are the generation number, the profit of the best solution x* found so far (until a feasible one appears, x* is the least infeasible individual), 1 if that x* is feasible and 0 if not, the smallest K_f + D_f on island F (0 means island F has a feasible individual), and the largest f_Q on island Q.
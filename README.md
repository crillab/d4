Authors: Lagniez Jean-Marie
Date: 2017/07/09

# How to compile

To compile and print out the help please use the following command lines:

```bash
cd core
make -j8
./d4 --help
```

To compile in debug mode (make possible the use of gdb and valgrind) and print out the help  please use:

```bash
cd core
make -j8 d
./d4_debug --help
```

To compile in profil mode (make possible the use gprof) and print out the help please use:

```bash
cd core
make -j8 d
./d4_profil --help
```


To compile in static mode and print out the help please use:

```bash
cd core
make -j8 rs
./d4_static --help
```



# How to run

To run the model counter:

```bash
./d4 -mc benchTest/littleTest.cnf
```

To run the dDNNF compiler:

```bash
./d4 -dDNNF benchTest/littleTest.cnf
```

To get the dDNNF in the file /tmp/test.nnf please use:

```bash
./d4 -dDNNF benchTest/littleTest.cnf -out=/tmp/test.nnf
cat /tmp/test.nnf
o 1 1 0
o 2 0
o 3 0
t 4 0
3 4 -2 3 0
3 4 2 0
2 3 -1 0
2 4 1 0
1 2 0
```

To get the certified dDNNF in the file /tmp/test.nnf enhance with the drat proof saved in /tmp/test.drat, please use:

```bash
./d4 -dDNNF benchTest/littleTest.cnf -out=/tmp/test.nnf -drat=/tmp/test.drat
cat /tmp/test.nnf
o 1 1 0
o 2 2 0
o 3 2 1 0
t 4 0
3 4 2 -2 3 0
3 4 2 2 0
2 3 2 -1 0
2 4 2 1 0
1 2 2 0
cat /tmp/test.drat
1 2 3 0
d 1 2 3 0
```

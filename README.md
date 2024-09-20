BPC-VRPTW
=========

This repository contains BPC-VRPTW, a rudimentary implementation of a branch-price-and-cut algorithm for the Vehicle Routing Problem with Time Windows (VRPTW). It contains a simple labeling algorithm based on Dijkstra's algorithm/uniform cost search. It also contains legacy separators for subset row and other inequalties that are not yet ported over to this code. Therefore, no cutting planes are implemented at this time.

License
-------

BPC-VRPTW is released under the GPL version 3. See LICENSE.txt for further details.

Dependencies
------------

BPC-VRPTW code uses SCIP for the branch-and-bound tree and requires a linear programming solver. Gurobi is the preferred linear programming solver but CPLEX and SoPlex are other options. Binaries for Gurobi are available under an [academic license](https://www.gurobi.com/downloads/end-user-license-agreement-academic/). Alternatively, SoPlex is open source and is included within this repository.

Compiling
---------

1. Download the source code by cloning this repository and all its submodules.
```
git clone --recurse-submodules https://github.com/ed-lam/BPC-VRPTW.git
cd BPC-VRPTW
```

4. Compile BPC-VRPTW.

    1. If using Gurobi, locate its directory. This directory should contain the `include` and `lib` subdirectories. Copy the path of the main directory into the command below.
    ```
    mkdir build
    cd build
    cmake -DGUROBI_DIR={PATH TO GUROBI DIRECTORY} ..
    cmake --build . --parallel
    ```

    2. If using CPLEX, locate the subdirectory `cplex` and copy the path to the `cplex` directory into the command below.
    ```
    mkdir build
    cd build
    cmake -DCPLEX_DIR={PATH TO cplex SUBDIRECTORY} ..
    cmake --build . --parallel
    ```

    3. If using SoPlex, compile using the command below.
    ```
    mkdir build
    cd build
    cmake ..
    cmake --build . --parallel
    ```

Usage
-----

After compiling, run BPC-VRPTW with:
```
./bpc-vrptw {PATH TO INSTANCE}
```

You can also set a time limit in seconds:
```
./bpc-vrptw —-time-limit={TIME LIMIT} {PATH TO INSTANCE}
```

The Solomon benchmarks can be found in the `instances/solomon` directory. Use the command below to try an instance.
```
./bpc-vrptw --time-limit=30 ../instances/solomon/c101_25.txt
```

Contributing
------------

This code is not actively developed. It serves as a template for academic research and experimentation. Potential contributors are welcome to discuss opportunities with the author.

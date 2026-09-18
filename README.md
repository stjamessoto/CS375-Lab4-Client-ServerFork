# CS375 Lab 4: Client-Server Application Using Fork in C and C++

Source files live in `src/`, one pair (or set) per exercise, since later
exercises modify the server/client behavior in ways that would otherwise
conflict if they all shared a single `server.c`/`server.cpp`.

## Setup (Ubuntu / WSL)

```
sudo apt update && sudo apt install build-essential
```

## Build & Run

Each exercise's server/client are built the same way: compile the C files
with `gcc`, the C++ files with `g++`, run the server in one terminal and
the corresponding client in another.

### Exercise 1 — Basic Fork-Based Server (C)
```
gcc -Wall -pthread src/ex1_server.c -o ex1_server
gcc -Wall -pthread src/ex1_client.c -o ex1_client
./ex1_server      # terminal 1
./ex1_client      # terminal 2
```

### Exercise 2 — Fork-Based Server (C++)
```
g++ -Wall -pthread src/ex2_server.cpp -o ex2_server
g++ -Wall -pthread src/ex2_client.cpp -o ex2_client
./ex2_server
./ex2_client
```

### Exercise 3 — Fork with Multiple Messages
```
gcc -Wall -pthread src/ex3_server.c -o ex3_server
gcc -Wall -pthread src/ex3_client.c -o ex3_client
./ex3_server
./ex3_client      # type messages, "exit" to quit
```

### Exercise 4 — Fork with Client Counter
```
g++ -Wall -pthread src/ex4_server.cpp -o ex4_server
./ex4_server
./ex2_client      # any number of times, in separate terminals
```

### Exercise 5 — Fork with Timeout
```
gcc -Wall -pthread src/ex5_server.c -o ex5_server
./ex5_server
./ex1_client      # then wait 10s with the connection idle to see the timeout
```

### Exercise 6 — Fork with Error Handling
```
g++ -Wall -pthread src/ex6_server.cpp -o ex6_server
./ex6_server
./ex2_client
cat server_errors.log   # created only if a socket/fork call fails
```

### Exercise 7 — Fork with Broadcast
```
gcc -Wall -pthread src/ex7_server.c -o ex7_server
gcc -Wall -pthread src/ex7_client.c -o ex7_client
./ex7_server
./ex7_client      # terminal 2
./ex7_client      # terminal 3, etc.
```

All 11 source files were compiled with `gcc`/`g++` `-Wall -Wextra` (zero
warnings) and smoke-tested end to end on Ubuntu/WSL before submission.

gcc ./src/master.c -o bin/master
gcc ./src/processA.c -lbmp -lm -lncurses -o bin/processA
gcc ./src/processB.c -lbmp -lm -lncurses -o bin/processB
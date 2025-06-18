:: TODO: accept args to build as RELEASE or DEV

IF NOT EXIST .\output\ ( mkdir .\output\ )

clang -g3 -march=native^
 -Wall -Wextra -Wshadow -Wundef^
 -pedantic^
 -Iinclude -Isrc^
 -DDEBUG^
 src/main.c -o output/interpreter.exe

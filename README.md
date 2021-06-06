# Interpeter
A C++17 interpreter for the Lox language specified in the book "Crafting Interpreters". This interpreter is using a simple hand-written recursive-descend parser and then uses a visitor-pattern interpreter to evaluate the AST.

This is mainly a learning project. The language is a simple C-like language but with dynamic typing and first-class functions.
I changed some things, so my version doesn't work exactly like the book. E.g. you cannot redefine variables, there are some builtins, and some keywords are optionally different.

# Installation
- `mkdir build && cd build`
- `cmake ..`
- `make`
- `./Lox` for REPL
- `./Lox <sourcefile>` for file interpretation

# Basic syntax
Works mostly as you would expect:
```
let x = 1; // Type deduced as double
print(x); // Built-in. Works for any build-in type (double, string, bool, functions, Null)

fn printSum(lhs, rhs) { print(lhs+rhs);}
fn call(func, arg1, arg2) {func(arg1, arg2);}
call(printSum, lhs, rhs); // Functions are first-class

while(x < 20) { print(true); x += 1; }
for(i = 0; i < 20; i = i + 1) { print(i); }

let cat = "k" + "i" + "tty" + true; // In the spirit of JavaScript mistakes, you even get overly ambitious implicit type conversions!
print(cat);

if(x == 20) 
{
  exit(); // Not required, but can be used for early exit
}
```

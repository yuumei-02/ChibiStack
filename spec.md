Copyright (c) 2026 yuumei-02. All Rights Reserved.
See the LICENSE file for more information.

# Note
The spec is still very much incomplete.
More will be added as the compiler implements more features.

# Literals
- int literals      ```69420```
- float literals    ```69.420```
- String literals   ```"zhyivannye miratte"```
- C string literals ```c"zhyivannye miratte"```

# Operators
## Arithmatic
* + - *
  idiv udiv

## Logical
* == !=
* && ||
* >  >=
* <  <=
* !

## Bitwise
* >> <<
* & |
* ^

# Instrinsics
## Stack operations
* Dup    Duplicate the element at the top of the stack
* Drop   Drop an item from the top of the stack
* swap   Swap the top of the stack with the item before it

## Syscalls
* syscall[0-6]   Perform a syscall with n number of arguments not including the syscall number

# Types
## Build-in types
* int
* uint
* ptr

## Typed pointers
A standalone ptr is untyped.
Pointers can be made typed by specifying it's type before it.
example
``` ChibiStack
int ptr ptr // Pointer to a Pointer to an int
```


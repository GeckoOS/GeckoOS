# GeckoOS Syntax Guide

By Ember2819

A simple, python-like programming language designed to run on GeckoOS.

Files are created with the `.gk` extension.

Run a script from the shell with:
```
gk yourprogram.gk
```

Or run the interactive demo with just:
```
gk
```

The absolute max is around 350 lines.

I promise I didn't just copy Python...

---

 
## Table of Contents
 
1. [Basic syntax rules](#basic-syntax-rules)
2. [Comments](#comments)
3. [Variables](#variables)
4. [Data types](#data-types)
5. [Operators](#operators)
6. [Strings](#strings)
7. [If / elif / else](#if--elif--else)
8. [While loops](#while-loops)
9. [For loops](#for-loops)
10. [Functions](#functions)
11. [Built-in functions](#built-in-functions)
12. [Limits](#limits)
 
---
 
## Basic syntax rules
 
- **Indentation defines blocks.** Use 4 spaces per level. Tabs count as 4 spaces.
- **One statement per line.** There is no semicolon or line-continuation syntax.
- **Case sensitive.** `myVar`, `myvar`, and `MYVAR` are three different names.
 
```
# correct
if x > 0:
    print(x)
 
# wrong -- missing indent
if x > 0:
print(x)
```
 
---
 
## Comments
 
Start a comment with `#`. Everything from `#` to the end of the line is ignored. There are no multi-line comments.
 
```
# this is a full-line comment
x = 10  # this is an inline comment
```
 
---
 
## Variables
You must assign a variable before using it.
 
```
name = "Ember"
count = 0
flag = True
```
 
Variable names must start with a letter or underscore and contain only letters, numbers, and underscores.
 
```
my_var = 1
_internal = 2
x2 = 3
```
 
---
 
## Data types
 
There are three data types.
 
### Integer
 
Whole numbers, positive or negative.
 
```
x = 42
y = -7
z = 0
```
 
### String
 
Text enclosed in single or double quotes. Both work.
 
```
a = "this is"
b = 'hard'
```
 
Escape sequences inside strings:
 
| Sequence | Meaning         |
|----------|-----------------| 
| `\n`     | newline         |
| `\t`     | tab             |
| `\\`     | literal `\`     |
 
```
msg = "line one\nline two"
path = "folder\\file"
```
 
### Boolean
 
`True` or `False`. (Case Sensitive)
 
```
running = True
done = False
```
 
---
 
## Operators
 
### Math (Scary)
 
| Operator | Description    | Example        | Result |
|----------|----------------|----------------|--------|
| `+`      | addition       | `3 + 4`        | `7`    |
| `-`      | subtraction    | `10 - 3`       | `7`    |
| `*`      | multiplication | `3 * 4`        | `12`   |
| `/`      | division       | `10 / 3`       | `3`    |
| `%`      | modulo         | `10 % 3`       | `1`    |
 
### Comparison
 
| Operator | Description           |
|----------|-----------------------|
| `==`     | equal                 |
| `!=`     | not equal             |
| `<`      | less than             |
| `<=`     | less than or equal    |
| `>`      | greater than          |
| `>=`     | greater than or equal |
 
Comparisons return `True` or `False`.
 
```
x = 5
print(x == 5)   # True
print(x > 10)   # False
```
 
### Logical
 
| Operator | Description                        |
|----------|------------------------------------|
| `and`    | true if both sides are true        |
| `or`     | true if at least one side is true  |
| `not`    | inverts a boolean                  |
 
```
x = 5
print(x > 0 and x < 10)   # True
print(x > 0 or x > 100)   # True
print(not True)            # False
```
 
### Unary minus
 
```
x = 10
y = -x   # y is -10
```
 
### String concatenation
 
Use `+` to join strings. If you combine a string with a non-string value it is converted automatically.
 
```
greeting = "Hello, " + "world!"
info = "count: " + 42        # "count: 42"
```
 
### Operator precedence (high to low)
 
1. unary `-`, `not`
2. `*`, `/`, `%`
3. `+`, `-`
4. `==`, `!=`, `<`, `<=`, `>`, `>=`
5. `and`
6. `or`
 
Use parentheses to override:
 
```
result = (2 + 3) * 4   # 20, not 14
```
 
---
 
## Strings
 
### Creating strings
 
```
s = "hello world"
t = 'single quotes work too'
```
 
### Concatenation
 
```
first = "Gecko"
second = "OS"
full = first + second   # "GeckoOS"
```
 
### Printing strings
 
```
print("hello")
print("value is: " + 99)
```
 
---
 
## If / elif / else
 
```
if condition:
    # runs if condition is true
 
elif other_condition:
    # runs if first was false and this is true
 
else:
    # runs if all conditions above were false
```
 
You can have zero or more `elif` branches and an optional `else`. They must be at the same indentation level as the `if`.
 
```
score = 75
 
if score >= 90:
    print("A")
elif score >= 80:
    print("B")
elif score >= 70:
    print("C")
else:
    print("F")
```
 
Falsy values (treated as false in conditions):
 
- `0`
- `False`
- `""` (empty string)
 
Everything else is truthy.
 
---
 
## While loops
 
Repeats a block as long as the condition is true.
 
```
while condition:
    # body
```
 
```
i = 0
while i < 5:
    print(i)
    i = i + 1
```
 
### break
 
Exits the loop immediately.
 
```
i = 0
while True:
    if i == 3:
        break
    print(i)
    i = i + 1
# prints 0, 1, 2
```
 
### continue
 
Skips the rest of the current iteration and goes back to the condition check.
 
```
i = 0
while i < 6:
    i = i + 1
    if i % 2 == 0:
        continue
    print(i)
# prints 1, 3, 5
```
 
---
 
## For loops
 
Iterates over a range of integers.
 
```
for variable in range(limit):
    # body
```
 
```
for i in range(5):
    print(i)
# prints 0, 1, 2, 3, 4
```
 
`break` and `continue` work inside `for` loops the same way as in `while`.
 
```
for i in range(10):
    if i % 2 == 0:
        continue
    if i > 7:
        break
    print(i)
# prints 1, 3, 5, 7
```
 
---
 
## Functions
 
### Defining a function
 
```
def name(param1, param2):
    # body
```
 
```
def greet(name):
    print("Hello, " + name)
```
 
### Calling a function
 
```
greet("Sifi11")
```
 
### Return values
 
Use `return` to send a value back to the caller. A function that reaches the end without a `return` returns `None`.
 
```
def add(a, b):
    return a + b
 
result = add(3, 4)
print(result)   # 7
```
 
### Parameters
 
Parameters are positional. The number of arguments must match the number of parameters exactly.
 
```
def power(base, exp):
    result = 1
    for i in range(exp):
        result = result * base
    return result
 
print(power(2, 8))   # 256
```
 
### Scope
 
Variables created inside a function are local to that function. Variables defined outside a function cannot be directly modified from inside a function.
 
```
def example():
    x = 99   # local to example()
    print(x)
 
example()
# print(x)  -- would give an error: x is not defined here
```
 
---
 
## Built-in functions
 
### print
 
Prints one or more values to the screen, separated by spaces, followed by a newline.
 
```
print("hello")
print(1, 2, 3)          # WILL NOT RUN use only one argument
print("value: " + 42)
```
 
### range
 
Used in `for` loops to generate integers from `0` to `n - 1`.
 
```
for i in range(10):
    print(i)
```
 
`range` is only valid as the iterable in a `for` statement. It cannot be stored in a variable or passed to a function.

### input

Reads a line of text typed by the user. Optionally prints a prompt first.
Returns the typed text as a string.

```
name = input("Enter your name: ")
print("Hello, " + name)
```

```
x = input()   # no prompt, just waits for input
print("You typed: " + x)
```

`input()` always returns a string. To use the result as a number, you would need to implement your own conversion logic.
 
---
 
## Limits
 
These limits exist because it uses a static array instead of heap allocation.
 
| Limit                        | Value      |
|------------------------------|------------|
| Max tokens per file          | 2048       |
| Max AST nodes                | 2048       |
| Max variables in scope       | 128        |
| Max function definitions     | 64         |
| Max parameters per function  | 8          |
| Max call depth               | 32         |
| Max string length            | 256        |
| Max source file size         | 8192 bytes |
| Max statements per block     | 64         |
 
---
 
## Full example
 
```
name = input("What is your name? ")
print("Hello from GeckoOS, " + name)

for i in range(5):
    print(i)

```

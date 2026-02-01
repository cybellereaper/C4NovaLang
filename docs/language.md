# NovaLang Language Guide

This guide documents NovaLang's minimal, expression-first surface syntax and
how each construct behaves. The current implementation favors stack-friendly
code generation: supported constructs lower to straightforward C expressions
and loops without heap allocations in generated code, keeping runtime state on
local stack frames whenever possible.

## 1) Modules and Imports

**Module declaration**

```nova
module demo.core
```

Every file starts with a `module` declaration that names the module path.

**Imports**

```nova
import demo.core
import demo.math { add, mul }
```

Imports accept an optional brace list to limit imported symbols.

## 2) Types

**Sum types (variants)**

```nova
type Option = Some(Number) | None
```

Sum types define tagged unions. Each variant may optionally carry parameters.

**Tuple types**

```nova
type Pair(Number, Number)
```

Tuple declarations describe a fixed-size aggregate of named types.

## 3) Declarations

**Functions**

```nova
fun identity(x: Number): Number = x
```

Functions are expressions. Parameters and return types are optional; the
semantic pass infers missing types.

**Let bindings**

```nova
let answer: Number = 42
```

Let bindings introduce immutable values at module scope.

## 4) Expressions

### Literals

```nova
42
"hello"
true
false
[1, 2, 3]
```

Numbers, strings, booleans, and lists are first-class expressions.

### Identifiers & Calls

```nova
answer
identity(42)
```

Calling an identifier produces a function invocation.

### Pipelines

```nova
1 |> identity |> double
```

Pipelines rewrite into nested calls, passing the previous value as the first
argument in the next stage.

### Blocks

```nova
{
    1
    2
    3
}
```

Blocks evaluate their expressions in order; the last expression is the block's
value. Semicolons may be used to separate expressions explicitly.

### Conditionals

```nova
if flag { 1 } else { 0 }
```

`if` is an expression; both branches are analyzed for type compatibility.

### While loops

```nova
while flag {
    tick()
}
```

`while` repeats its body while the condition is true and always returns `Unit`.
The condition must be `Bool`.

### Match expressions

```nova
match value {
    Some(x) -> x
    None -> 0
}
```

`match` performs variant pattern matching and must be exhaustive for sum types.

### Async, Await, and Effects

```nova
async { 42 }
await task
! call_impure()
```

- `async` marks an expression as asynchronous.
- `await` waits on asynchronous expressions.
- `!` flags impure effects, which are tracked in semantic analysis.

### Lambdas

```nova
(x: Number) -> x
```

Lambda expressions define anonymous functions with optional type annotations.

## 5) Turing Completeness

NovaLang is Turing-complete through:

1. **Recursive functions**, since functions are first-class values.
2. **While loops**, which provide unbounded iteration.

## 6) Stack-Friendly Execution Model

The compiler emits C that relies on local variables and direct expressions for
supported constructs. Because there is no runtime heap allocation in generated
code for the currently supported IR features, programs naturally keep their
working set on the stack. This makes NovaLang well suited for small, predictable
programs and embedded-style workflows.

## 7) Recommended Style Rules

- Keep blocks short and use pipelines to express linear transformations.
- Prefer `match` over nested `if` when destructuring variants.
- Use explicit type annotations at module boundaries for clarity.

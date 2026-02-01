# NovaLang Reference (Clean Core)

NovaLang is an expression-first language with a compact core: modules, typed
functions, algebraic data types, and pure/async effects. This reference mirrors
`nova.g4` and documents the minimal surface syntax so the language stays tidy,
predictable, and friendly to stack-based execution.

## 1) Program layout

```
module my.app
import core.math
import core.io { read, write }

# declarations
```

**Rule:** a file must start with `module`, then any number of `import` statements,
then declarations (`type`, `fun`, `let`).

## 2) Modules & imports

| Form | Purpose |
| --- | --- |
| `module a.b.c` | Declare the module path for the file. |
| `import x.y` | Import an entire module. |
| `import x.y { foo, bar }` | Import specific symbols. |

## 3) Declarations

### 3.1 Types

```
type Option = Some(Number) | None
```

* **Variants** are separated by `|`.
* A variant may have a payload list (`Some(Number)`), or no payload (`None`).

### 3.2 Functions

```
fun add(x: Number, y: Number): Number = x
```

* `fun` introduces a named function.
* Parameters may include type annotations; the return type is optional.
* The body is a single **expression** (blocks are expressions too).

### 3.3 Let bindings

```
let answer: Number = 42
```

* `let` defines a value at the top-level.
* The right-hand side is always an expression.

## 4) Expressions (methodized)

Every construct yields a value. The forms below are listed in evaluation order
(from highest priority to lowest):

1. **Primary forms**
   * Literals: `42`, `true`, `"text"`, `[1, 2, 3]`
   * Identifiers: `name`
   * Parentheses: `(expr)`
   * Lambdas: `(x: Number) -> x`
2. **Calls**
   * `f(1, 2)`
3. **Pipelines**
   * `value |> step |> other(1)`
   * The left value becomes the first argument of each stage.
4. **Unary effects**
   * `async { expr }` marks async work.
   * `await expr` waits on async work.
   * `!expr` marks an impure expression.
5. **Match**
   * `match value { Some(x) -> x; None -> 0 }`
6. **If / else**
   * `if condition { expr } else { expr }`

## 5) Blocks and sequencing

```
{
    expr1;
    expr2;
    expr3
}
```

* A block is an expression.
* Semicolons sequence expressions; the final expression is the block value.
* Blocks enable local sequencing without introducing a new declaration form.

## 6) Effects model

| Marker | Meaning |
| --- | --- |
| `async { ... }` | Adds the async effect to the expression result. |
| `await expr` | Reads an async value (propagates effects). |
| `!expr` | Marks impure computation to make effects explicit. |

**Rule:** effect tags propagate through function calls and pipelines so tools
can track purity and async usage.

## 7) Turing completeness (in the core)

NovaLang’s core is Turing complete by combining:

* **Conditional branching** (`if`, `match`).
* **General recursion** (functions can call themselves or each other).

No dedicated loop form is required, keeping the surface syntax minimal while
preserving full computational power.

## 8) Stack-oriented execution model

NovaLang favors stack-friendly evaluation:

* Expressions are value-producing; most temporary values are scoped to a block
  or function and can be stack-allocated by downstream codegen.
* Impure or async operations are explicit via effects, helping compilers keep
  pure values in registers/stack until needed.
* Lists and strings remain heap-backed; scalar values and unit-like results are
  intended to stay on the stack.

## 9) Minimality checklist

When adding language features, aim for:

1. **Single-expression bodies** (prefer blocks over new statement forms).
2. **Composable primitives** (match + recursion over specialized loops).
3. **Clear effects** (prefer explicit markers over implicit side effects).
4. **Small syntax surface** (avoid redundant syntax or overlapping features).

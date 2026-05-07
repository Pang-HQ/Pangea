## Code Style

The goal of this style is clarity, consistency, and ease of maintenance.
Formatting should make code easier to scan, review, and modify.

This codebase prefers flat, readable control flow. Exceptional cases should be
handled early when doing so makes the main path easier to follow.

These rules are intentionally simple. When a case is not covered here,
prefer readability, consistency with nearby code, and the least surprising
form.

## General principles

- Favor clarity, consistency, and local readability over cleverness or density.
- Follow the style of this codebase consistently.
- Write code for readers first.
- Prefer simple, predictable formatting over compact or decorative formatting.
- Do not introduce a new local style within the same file.
- Prefer flat control flow over unnecessary nesting.
- Handle invalid, exceptional, and early-exit cases early when doing so makes
  the main path clearer.
- Keep the main path of execution visually prominent.

## Line length

- Code has a soft limit of 100 columns and a hard limit of 120 columns.
- Most lines should naturally fit within 80 columns.
- Comments have a hard limit of 80 columns per line.
- Line wrapping must never reduce readability.
- A line may exceed tahe limit when breaking it would make the code less clear.
- Do not compress code to satisfy a line limit.

## Modularity

- This codebase is designed to be modular, and is clearly split into subsystems.
- Headers should expose only the minimal, stable, public interface required by
  other subsystems. Internal helpers should remain in source files or private
  headers.
- Each file should represent a single, cohesive abstraction.
- Files have a soft limit of 600 LOC. Exceeding this limit should be discussed
  for potential refactoring into smaller abstractions if necessary.

## Indentation

- Use 4 spaces per indentation level.
- Tabs are not allowed.
- Indentation must be consistent within a file.
- Indent nested preprocessor directives by one space per nesting level.
- Continuation lines should be indented clearly relative to their parent line.
- Wrapped lines should make the structure of the statement obvious.

## Whitespace

- Use whitespace to separate logical units, not for visual decoration.
- Use a single space after keywords before parentheses.

```c
if (cond) {
    work();
}
```

- Do not put spaces just inside parentheses, brackets, or angle brackets.

```c
f(a, b);
arr[i];
Vec<int>;
```

- Use spaces around binary operators.

```c
a = b + c;
ok = x == y;
```

- Do not use spaces around unary operators unless required for clarity.

```c
i++;
*p;
&value;
```

- No trailing whitespace at the end of a line.
- Files must end with a single newline.
- Do not use multiple consecutive blank lines.

## Blank lines and layout

- Use a single blank line to separate logical sections.
- Avoid excessive vertical spacing.
- Do not insert blank lines inside tightly related code.
- Use blank lines to make function bodies easier to skim.
- Keep visually related declarations and statements together.

## Pointer formatting

- Write pointers as `T *p`, not `T* p`.
- The * binds to the declarator, not the type.
- Apply this consistently in declarations, parameters, fields, and casts.
- Do not mix pointer styles within a file.

```c
const char *name;
void *data;
int *items;
```

- This same style applies to references.

## Declarations

- Keep declarations easy to read at a glance.
- Prefer one declaration per line when doing otherwise could mislead.
- Do not combine unrelated declarations on one line.

```c
int *a;
int *b;
int count;
```

- Prefer declarations close to first use when practical.
- Keep qualifiers and types in a consistent order.

## Control flow and braces

- Opening braces go on the same line as the controlling statement or
  declaration.

```c
if (cond) {
    work();
}

while (ready) {
    poll();
}
```

- Closing braces go on their own line, aligned with the opening statement.

```c
if (cond) {
    work();
}
```

- Prefer shallow control flow.
- Avoid unnecessary nesting.
- When an early return, continue, or break makes the code simpler, prefer it
  over introducing another level of indentation.
- Do not use an else after a branch that returns, breaks, or continues.
- Keep the main path of a function at the outermost indentation level whenever
  practical.

- Always use braces for:
  - multi-line bodies,
  - any if statement with an else,
  - nested control flow,
  - bodies that contain comments,
  - bodies likely to grow,
  - any case where omission would reduce clarity.

- A single-line body without braces is generally acceptable only for simple
  early-exit statements such as return, break, and continue, when the result is
  clearly more readable.

```c
if (!ptr)
    return;

if (done)
    break;
```

- Braceless single-line bodies are mainly for guard-clause style early exits,
  not for ordinary work.

- For ordinary work statements, assignments, calls, and other nontrivial
  bodies, braces are usually preferred even when the body fits on one line.

```c
if (ready) {
    x = y;
}

while (it) {
    it = it->next;
}
```

- Do not use brace omission for long or complex statements.

```c
if (very_long_condition && another_condition)
    do_complicated_work(arg1, arg2, arg3, arg4);
```

  ^ The above should use braces.

- Prefer braces when there is any doubt.

## Conditions and expressions

- Keep conditions readable and visually obvious.
- Prefer simple conditions over dense combined expressions.
- Break complex expressions across lines instead of compressing them.
- Prefer a clearer multi-line form over a dense one-line expression.
- Use parentheses when they improve readability, even if not strictly needed.
- Do not rely on operator precedence when the grouped meaning is not obvious.
- Avoid deeply nested conditional expressions.
- Prefer introducing a temporary with a clear name when it makes a condition
  easier to read.
- Prefer control flow that keeps the main path visually obvious.
- When a condition handles an invalid, exceptional, or early-exit case, prefer
  a guard clause over adding another level of nesting.
- Do not combine multiple unrelated checks into a single dense condition when
  separate guard clauses would be clearer.

- Do not use increment or decrement operators inside conditions or complex
  expressions.
- Avoid expressions with hidden side effects.
- Prefer separating state changes from control flow.

## Nesting and guard clauses

- Prefer flat control flow over unnecessary nesting.
- Use guard clauses to handle invalid, exceptional, and trivial early-exit
  cases when they make the function easier to read.
- Avoid else blocks after a branch that exits the current scope.
- Keep the main behavior of a function at the outermost indentation level
  whenever practical.
- If a function becomes deeply nested, first consider whether a guard clause or
  helper function would make it clearer.
- Do not use guard clauses when they make cleanup, ownership, or control flow
  harder to follow.

## Functions

- Keep functions visually structured and easy to skim.
- Separate distinct steps with blank lines when it improves readability.
- Break long parameter lists in a way that makes their structure obvious.
- Prefer layouts that make names, arguments, and nesting easy to scan.
- Keep return paths and cleanup paths visually clear.
- Prefer function structure that keeps the happy path easy to follow.
- Avoid wrapping the main logic in unnecessary conditionals.
- If a function becomes too nested, consider extracting a helper.

## Function declarations and calls

- Keep short declarations and calls on one line when they remain readable.
- Break long parameter lists one per line when needed.
- Indent wrapped parameter lists clearly and consistently.

```c
void process_item(
    Context *ctx,
    const std::uint8_t *ptr,
    std::uint64_t len,
    std::uint32_t flags);
```

- Preserve readability when wrapping chained or nested calls.

## Switch statements

- Indent case labels one level inside the switch.
- Indent case bodies one level inside the case.
- Keep fallthrough explicit when intentional.

```c
switch (kind) {
    case Token_A:
        handle_a();
        break;

    case Token_B:
        handle_b();
        break;

    default:
        fail();
        break;
}
```

- Use an explicit comment for intentional fallthrough where needed.

## Comments

- Comments should explain why, not restate what the code already says.
- Keep comments concise, accurate, and maintained with the code.
- Prefer clear code over explanatory comments where possible.
- Write complete, readable comments.
- Keep comment lines at or under 80 columns.
- Use comments for intent, invariants, assumptions, ownership, and tricky edge
  cases.
- Do not leave stale comments in the code.
- Prefer // comments for single-line comments.
- Prefer /* ... */ comments for multi-line comments.
- Do not switch comment styles without a clear reason.

Multi-line block comments should be formatted like this:

```c
/*
 one space here
 one space again
*/
```

Single-line comments should be formatted like this:

```c
// one space here again
```

## Inline comments

- Use inline comments sparingly.
- Keep inline comments short and directly relevant to the line they annotate.
- Separate inline comments from code with at least two spaces when practical.
- Inline comments should use //.

```c
weak_count += 1;  // includes implicit weak reference
```

- Do not use inline comments to explain obvious code.

## Naming and local consistency

- Prefer names that make code self-explanatory.
- Use British English spelling for all project-defined identifiers and
  comments (e.g. colour, initialise, behaviour, tokenise).
- Do not mix British and American spelling within the codebase.
- External APIs, libraries, and standards must retain their original naming.
- Do not abbreviate unless the abbreviation is well understood in the codebase.
- Keep related names visually and semantically consistent.
- Match the surrounding style for local naming and layout.

### Naming conventions

* Use PascalCase for classes, structs, enums, and namespaces:
```c
class SourceBuffer;
struct Token;
enum class TokenType;
```

* Use snake_case for functions, methods, variables, and parameters:
```c
void do_something();
std::uint32_t offset;
```

* Member variables use snake_case.

  - Public data members use plain snake_case:
    ```c
    std::string filename;
    std::uint32_t line;
    ```

  - Private or internal members use snake_case with a trailing underscore:
    ```c
    float weight_kg_;
    std::uint8_t age_;
    ```

* Use UPPER_CASE for constants (macros, global constants), and enum values:
```c
#define MAKE_VALUE(x) vec.push_back(std::move(x))
TokenType::KW_IF
```

* About `constexpr` specifically:
  - `constexpr` functions follow function naming (snake_case)
  - `constexpr` variables follow constant naming (UPPER_CASE)

## Error handling and early returns

- Prefer straightforward control flow.
- Early returns are encouraged when they simplify the function.
- Use guard clauses for invalid state, failed preconditions, and error paths
  when this keeps the main logic easier to read.
- Handle failure first, then continue with the normal path.
- Keep error paths visually obvious.
- Do not hide important control flow in dense expressions.
- Do not wrap the main path in an else block after an early return.
- Do not force guard clauses when they would duplicate cleanup or make
  ownership and lifetime rules harder to follow.

```c
if (!ptr) {
    return nullptr;
}
```

## Alignment

- Do not align code purely for appearance.
- Do not add spacing just to make tokens line up vertically.
- Prefer formatting that stays stable under edits.

  Avoid patterns like this:

```c
int      a;
uint64_t count;
char    *name;
```

  Prefer:

```c
int a;
uint64_t count;
char *name;
```

## Includes

- Keep include ordering consistent.
- Prefer project headers before system headers when that matches the existing
  codebase style.
- Group includes logically.
- Do not leave unused includes in a file.

## Formatting judgment

When a formatting choice is not covered here, apply the following order:

1. readability,
2. consistency with nearby code,
3. the least surprising form.

If a rule and readability appear to conflict, prefer the clearest form that
still respects the spirit of the style.

## Enforcement

- All contributed code must follow this style.
- Code that does not follow this style may be rejected during review.
- When in doubt, prefer consistency with the surrounding file and the simpler
  formatting choice.
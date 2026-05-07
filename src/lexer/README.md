# Lexer

Language-level rules for how source text breaks into tokens.

## Newlines

A newline acts as a statement terminator, like `;`.

- Accepted line endings are LF, CR, and CRLF.
- Leading blank lines, before the first token, are ignored.
- Consecutive blank lines collapse into a single terminator. The number of
  physical newlines collapsed is preserved so a formatter can distinguish a
  plain terminator from a paragraph break.
- A newline inside `()`, `[]`, or `{}` is not a terminator; it is whitespace.

## Comments

Comments are whitespace and never separate statements.

- `// ...` line comment, runs to the end of the line.
- `/* ... */` block comment. Block comments may nest.

## Strings

Two forms:

- `"..."` plain string.
- `c"..."` C-string. Guaranteed NUL-terminated for C interop.

Rules:

- A string literal must lie on a single line. A literal newline inside the
  quotes is an error.
- Line continuations (`\` followed by a newline) are not supported.
- A literal NUL byte (0x00) appearing inside the quotes is an error.

Escape sequences:

- `\a \b \e \f \n \r \t \v` standard control escapes.
- `\\ \' \" \0 \?` single-character escapes.
- `\xHH` one byte from one or two hex digits.
- `\uHHHH` one BMP code point, re-encoded as UTF-8. Surrogates are an error.
- `\UHHHHHHHH` one Unicode code point, re-encoded as UTF-8. Surrogates and
  values above U+10FFFF are errors.

A backslash followed by anything not in the list above, or by nothing, is
an error.

## Numbers

A numeric literal must start with an ASCII digit.

```
5         -> integer
5.        -> float
5.5       -> float
5e10      -> float
5.5e-2    -> float
.5        -> '.' then integer 5    (no leading-dot floats)
42.foo    -> one numeric literal: float core "42." with invalid suffix "foo"
```

### Bases

```
0x1F      -> integer (hex)
0b1010    -> integer (binary)
0o17      -> integer (octal)
```

A base prefix with no digits after it is an error. A base prefix followed
by a character invalid for that base is an error.

### Digit separators

`_` is allowed between two valid digits of the same part of the literal.

```
1_000_000   ok
_5          invalid (leading separator)
5_          invalid (trailing separator)
5__0        invalid (consecutive separators)
0x_FF       invalid (separator before first digit)
1_.5        invalid (separator across the decimal point)
1.5_e2      invalid (separator across the exponent marker)
```

### Suffixes

- An integer suffix (e.g. `i32`, `u64`) attaches to any integer literal,
  including non-decimal.
- A float suffix (e.g. `f32`, `f64`) attaches only to decimal float
  literals.
- Any other identifier in suffix position is an invalid suffix.

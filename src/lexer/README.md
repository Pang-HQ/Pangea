# Lexer

One-shot, single-buffer lexer producing a flat token stream plus an
out-of-line trivia table. See `lexer.h` for the API; this document
covers the policies (newlines, trivia, strings, numbers).

## Newline policy

- `SPECIAL_NEWLINE` acts as a statement terminator like `;`.
- It is emitted only when the bracket stack is empty - newlines inside
  `()`, `[]`, and `{}` stay classified as trivia.
- Leading blank lines (before any real token) are ignored.
- Consecutive blank lines collapse into a single `SPECIAL_NEWLINE`
  token; the count of physical newlines collapsed is stored in the
  token's payload and exposed via `newlines_of(const Token &)` so a
  formatter can distinguish a plain terminator from a paragraph break.

This keeps multi-line bracketed expressions free of synthetic
terminators while preserving line-based statement splitting.

Future: allow soft continuation after tokens that still expect a RHS
(e.g. a binary operator at end of line).

## Comment / trivia model

Comments are kept out-of-line on `Lexer::Output::trivia` and linked to
their owning token through a sparse `trivia_attachments` table. Tokens
without comments pay no per-token overhead.

Each `TriviaAttachment` stores the *first* index of a contiguous run on
each side; the run extends up to (but not including) the next trivia
index claimed by any later attachment, or the end of the trivia vector.
A debug-only `TriviaTable::audit()` pass at the end of `tokenise()`
checks that the run starts are unique and cover index 0 when any
trivia exists.

Attachments are built into a single in-flight slot on `TriviaTable`
(`pending_token_` / `pending_attach_leading_` / `pending_attach_trailing_`)
and only sealed onto the output vector when the token can no longer
gain trivia. `TriviaTable::finalise()` (called by the lexer after the
final token) seals any open attachment and locks the table; the
`move_*` accessors assert the table has been finalised.

Same-line block comments after the line's last real token are held in
`held_block_trivia_` and then either:

- attached as **trailing** trivia of that token at the next line ending, or
- attached as **leading** trivia of the following real token if it
  appears before the line ends.

Line comments after a real token attach as trailing immediately. Line
or block comments that appear before any real token on a line are
queued as leading trivia for the next token (or for `SPECIAL_EOF` if
no further token arrives).

User-facing documentation: `docs/user/lexical/literals_and_comments.md`.

## String model

- Plain strings are length-tracked and pooled *without* a trailing
  NUL.
- `c"..."` strings are pooled with an extra trailing NUL that is **not**
  counted in `ref.length`, so a callee that needs a NUL-terminated
  pointer can use `pool.view(ref).data() + ref.length` and find a NUL
  there.
- Escape decoding lives in `unicode/unicode_escape`; the lexer pass
  only finds the literal's byte bounds and handles recovery.
- Strings are single-line. A literal newline inside a string -
  including `\<newline>` (line continuation, not supported) - bails
  with a specific diagnostic instead of letting the literal silently
  span to the next quote.
- Embedded NUL bytes inside the source produce a dedicated
  `LEXER_EMBEDDED_NUL` diagnostic; if recovery also fails to find a
  closing quote, the message says so.

## Number model

`Lexer::scan_number` only captures the byte span and delegates to
`parse_numeric` (declared in `detail/numeric/numeric.h`, implemented
in `detail/numeric/`). The error taxonomy is `NumericParseError`; the
internal segments view is `NumericSegments`.

### Grammar

- A literal starts with an ASCII digit; `.5` is not a float literal.
- `42.` is a float literal; `42.foo` is one numeric literal with float
  core `42.` and invalid suffix `foo`.
- Underscores must sit between two valid digits for that part of the
  literal.
- Integer literals (including non-decimal) accept integer suffixes;
  decimal floats accept float suffixes.

### Implementation policy

- Decimal floats are decoded by `fast_float` after stripping `_`.
- After `0x` / `0b` / `0o`, EOF means `NO_DIGITS`; a bad first
  post-prefix character means `MALFORMED`.
- Overflow keeps consuming digits so the diagnostic span covers the
  whole literal.

User-facing documentation: `docs/user/lexical/literals_and_comments.md`.

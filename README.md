# mygrep

A simple grep-like command-line tool implemented in C++ that supports basic regular expression features including character classes, quantifiers, alternation, backreferences, and anchors.

---

## Features

- Supports basic regex constructs:
  - Digit (`\d`) and word character (`\w`) classes.
  - Character classes: `[abc]` and negated character classes: `[^abc]`.
  - Quantifiers: `+` (one or more), `?` (zero or one).
  - Alternation: `(a|b|c)`.
  - Backreferences: `\1`, `\2`, etc. for captured groups.
  - Anchors: `^` (start of line) and `$` (end of line).
- Validates regex patterns and throws detailed errors for malformed patterns (e.g., unmatched brackets).
- Reads a single input line from standard input and searches for a match against the given pattern.
- Returns exit code `0` if the input matches the pattern, `1` otherwise.
- Prints helpful error messages for invalid patterns or incorrect usage.

---

## Usage

Run the executable with two arguments: the flag `-E` followed by the regex pattern. Provide the input line via standard input.

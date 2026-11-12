# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this project is

A C++ source-code obfuscator implemented entirely in `main.cpp` (single translation unit, no external libraries). It preprocesses the input with `g++ -E -P`, replaces every distinct token (or every normalized line) with a generated identifier, and writes a file consisting of `#define <alias> <original>` directives followed by a stream of aliases. The output only becomes valid C++ when the preprocessor expands those aliases; compiling it restores the original program.

The README (Chinese) documents usage; `main.cpp` is the source of truth for exact behavior.

## Build, run and test

There is no build system and no lint config. `setup.sh` installs the binary; `tests/run.sh` is the regression suite.

```bash
./setup.sh                                   # -O2 -static, installs to /usr/local/bin/obfuscator_cpp (sudo; removes existing binary)
g++ -O2 -static main.cpp -o obfuscator_cpp   # equivalent build without installing
./obfuscator_cpp <input.cpp> <out.cpp> [options]
./tests/run.sh                               # build + run the regression suite (uses g++)
```

`tests/run.sh` obfuscates every `tests/cases/*.cpp` in token, `-l`, `-n`, `-x` and `-t` mode, compiles and runs the result, checks mode shape and sidecar cleanup, and asserts argument/IO error codes. `CXX` and `STD` override the compiler and standard (defaults `g++` and `-std=c++20`).

At runtime the tool invokes `g++ -E -P` through `system()`, so `g++` must be on `PATH`; only POSIX systems are supported. Paths embedded in shell commands are single-quoted (`shell_quote`), so spaces, `$`, quotes and backticks in filenames are treated as data. Sidecar files `<out>.tmp`, `<out>.raw` and `<out>.exp` are created next to `out_path` and removed on success.

### Options

| Flag | Behavior |
|---|---|
| `-s "s1 s2 ..."` | Space-separated symbol set (default `OOO OO1 O1O O11`). Requires at least 2 distinct symbols; each must be a valid identifier and all must have the same length, so alias concatenation is injective. |
| `-h <file>` | Prepend the file's contents (e.g. a signature header) to the output. Identifiers in it are excluded from alias generation. |
| `-l` | Line mode: alias each whitespace-normalized source line instead of each token. |
| `-m <n>` | Aliases per output line (default 7, must be ≥ 1). |
| `-M <n>` | Maximum characters per output line (default unlimited, must be ≥ 1). |
| `-x "<set1>" "<set2>"` | Two-pass: token obfuscation with set1, then line obfuscation with set2. Both sets obey the `-s` constraints. |
| `-t` | Translate/deobfuscate: output the macro-expanded (`g++ -E -P`) source instead of obfuscating. Also works on an already obfuscated file. |
| `-n` | No aliasing: re-emit tokens/lines with normalized spacing only. |

Both positional paths are required; omitting either exits 11.

### Exit codes

| Code | Meaning |
|---|---|
| 11 | Bad arguments / missing input or output path |
| 12 | `-s` value missing, or invalid symbol set |
| 13 | `-h` value missing |
| 14 | Head file open/read failure |
| 15 | `-m` value missing or invalid |
| 16 | `-x` values missing, or invalid symbol set |
| 17 | `-M` value missing or invalid |
| 18 | More than two positional arguments |
| 19 | Unknown option |
| 21 | Input file open/read failure |
| 22 | `<out>.tmp` open/write failure |
| 23 | `<out>.exp` open failure |
| 24 | `g++ -E -P` failure, or `.exp`/`.raw` I/O failure |
| 25 | Output file open/write failure |
| 31 | `-x` intermediate `<out>.tmp` open/write failure |
| 32 | `-x` intermediate `<out>.tmp` read/remove failure |
| 33 | `<out>.exp` removal failure |

### Verifying a change

```bash
./tests/run.sh

# Manual smoke test:
printf '#include <iostream>\nint main(){ std::cout << "hi\\n"; }\n' > /tmp/in.cpp
./obfuscator_cpp /tmp/in.cpp /tmp/out.cpp && g++ /tmp/out.cpp -o /tmp/out && /tmp/out
./obfuscator_cpp /tmp/out.cpp /tmp/restored.cpp -t && g++ /tmp/restored.cpp -o /tmp/restored && /tmp/restored
```

Also exercise `-l` and `-x`, since they go through `obfuscate_line()` and the two-pass pipeline rather than plain `obfuscate()`.

## Architecture

Flow in `main.cpp`:

1. `main()` parses args into globals (`no_chars`, `max_line_chars`), `mode` (0 = token, 1 = line, 2 = two-pass, 3 = translate), `line_count = 7`, and input/output paths. It validates the paths, symbol sets and numeric options before touching the filesystem.

2. `expand_file()` is the mandatory preprocessing stage. It reads the whole input, strips a UTF-8 BOM, and walks the lines with a lexical state machine (`scan_state`) that tracks block comments, raw strings and line continuations. Only real `#include` / `#include_next` directives at directive position are recognized (leading whitespace, `#  include`, and comment prefixes are allowed). Each include is replaced in `<out>.tmp` by a unique standalone string marker; the text before the `#` (whitespace/comments) is kept so a block comment still closes, and an unclosed block comment opened after the include operand is kept in tmp as well. The rest of the file is run through `g++ -E -P -x c++ <out>.tmp -o <out>.raw`, then every marker that survived conditional evaluation is restored to the directive text in `<out>.exp`. Markers under `#if 0` disappear naturally, and `#include` text inside raw strings or comments is never touched. Only a line consisting solely of a marker is restored, so user text cannot spoof a marker. The obfuscator therefore sees fully macro-expanded code: user `#define`s and input macros no longer exist at obfuscation time, while includes remain as directives.

3. Token mode — `obfuscate()` + `tokenize_stream()`: hand-written scanner. `scan_pp_number()` keeps preprocessing numbers whole (`1.009`, `.5`, `5.`, `1e-9`, `0x1.8p+3f`, `1'000'000`, `0b1010'1010u`, `42_km`). `literal_prefix()` / `scan_quoted_literal()` handle encoding prefixes (`u8`, `u`, `U`, `L`), raw strings (`R`, `u8R`, `uR`, `UR`, `LR`) including multi-line delimiters, user-defined literal suffixes (`"abc"sv`, `'x'_c`, `R"(x)"_r`), and odd/even backslash escaping. `scan_identifier()` accepts universal-character-names (`\uXXXX`, `\UXXXXXXXX`, `\N{...}`), `$` (GCC extension) and non-ASCII bytes, so the UCNs emitted by `g++ -E` for non-ASCII identifiers survive. The operator table includes `...`, `<=>` and the digraph tokens `<:`, `:>`, `<%`, `%>` (with the C++ rule that `<::` is `<` + `::`). `#...` lines are kept verbatim (continuing lines ending in `\` are joined), blank lines are skipped, and comments are already stripped by `g++ -E` (the `//` handling is defensive). Tokens that cannot be object-like macro replacements — `(`, `)`, `,` and multi-line raw strings — are emitted literally: function-like macros from headers (`va_start`, `assert`, `offsetof`, …) only invoke when the next token is a real `(`, and their arguments only split on real `,`/`)`.

4. Line mode — `obfuscate_line()`: trims each line and collapses whitespace outside literals and pp-numbers, keeps `#...` lines verbatim, and treats each distinct normalized line as one aliasable item. Raw-string blocks are emitted verbatim (byte-for-byte, including newlines) so their content and compilation are preserved. Lines containing a raw string, lines inside a multi-line parenthesized region, and lines whose first token is `(` are also emitted verbatim; this keeps multi-line function-like macro invocations visible to the preprocessor. `count_parens()` supplies the nesting depth; `find_unclosed_raw()` handles a second raw string starting on the same or closing line.

5. `create_mapping()` / `generate_id()` assign aliases: fixed-length strings in base K over the symbol set, with length minimal such that K^L ≥ N, skipping names in a forbidden set (all input tokens, identifiers in preprocessor lines, and identifiers in the `-h` head). If the candidate space is exhausted, L grows and assignment retries. Because symbols have equal length, distinct indices always produce distinct aliases. Assignment follows `std::unordered_set` iteration order, so exact alias names and output bytes are implementation-defined.

6. Mode 2 (`-x`) chains the two passes via `<out>.tmp`; mode 3 (`-t`) writes the expanded input unchanged.

## Gotchas when modifying

- The alias output is not standalone C++ — it relies on the emitted `#define`s. Keep definitions consistent with the aliased stream and emitted before use.
- Alias-to-token assignment follows `std::unordered_set` iteration order, so exact alias names and output bytes are implementation-defined. Avoid golden-output tests that assume deterministic naming.
- Aliases are checked against input tokens, preprocessor lines and the `-h` head, but not against macros defined by included headers (headers are not expanded). A collision with a header macro remains theoretically possible; changing the symbol set is the workaround.
- Preprocessing happens without the included headers, so conditional compilation guarded by header-defined macros can evaluate differently than a real build, and includes guarded by such macros can be dropped. Predefined compiler macros (e.g. `_WIN32`) work correctly. Macro-form includes (`#define HDR <x>` followed by `#include HDR`) are not supported because the defining directive is consumed by preprocessing.
- Includes are restored at their original position in `<out>.exp`, but the obfuscators hoist all `#...` lines to the top of the generated stream, so the final position can differ from the input.
- Token mode never aliases `(`, `)`, `,` or multi-line raw strings. Line mode also leaves raw-string blocks and the verbatim regions described above unobfuscated. Do not "fix" this without re-checking function-like macro invocation and raw-string round-tripping.
- Symbol sets must contain at least 2 distinct, equal-length, valid identifiers. Mixed-length symbols would make alias concatenation non-injective; duplicates would generate duplicate macro names.
- Paths used in shell commands are single-quoted. Keep using `shell_quote()`/`rm_command()`/`gpp_command()` for any new `system()` call.

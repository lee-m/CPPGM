## CPPGM Programming Assignment 2 (posttoken)

### Overview

Write a C++ application called `posttoken` that accepts a _C++ Source File_ on standard input that does NOT include any...

 - preprocessing directives
 - pre-defined macro names
 - the pragma operator
 
...executes phases 1, 2, 3, 4, 5, 6 and the tokenization part of 7 of the _Phases of Translation_, and describes the resulting sequence of lexically analyzed `tokens` to standard output in the specified format.

Notice that by virtue of the restrictions, phase 4 (preprocessing) is a _no-op_ (does nothing).

### Prerequisites

You should complete Programming Assignment 1 before starting this assignment.

### Starter Kit

The starter kit can be obtained from:

    $ git clone git://git.cppgm.org/pa2.git

It contains a stub implementation of `posttoken`, a compiled reference implementation and a test suite.

You will also want to reuse most of your code from PA1.

### Input Format

The C++ Source File shall be read from standard input `std::cin` in UTF-8 format as per PA1.

### Error Reporting

If an error occurs in phases 1, 2 or 3 `main` should `return EXIT_FAILURE` as per the behaviour of `pptoken` in PA1.

If an error occurs during string literal token concatenation (phase 5) because of incompatible string literals, you should also `return EXIT_FAILURE`.

If an error occurs while converting a `preprocessing-token` to a `token` than you should output an `invalid` token (see below) and continue.

Preprocessing-tokens `#`, `##`, `%:`, `%:%:`, `non-whitespace-characters`, and `header-nams`, should be output as `invalids`.

If a `preprocessing-token` contains a pre-defined macro names or the pragma operator you may treat them as identifiers.

### Restrictions

As per PA1

### Output Format

`posttoken` shall write to standard output the following in UTF-8 format:

For each token in the sequence, one line shall be printed consisting of space-character separated elements.

The first element is the _Token Type_, one of:

    simple
    identifier
    literal
    user-defined-literal
    invalid

The second is the UTF-8 encoded _Token Data_ from PA1 (the tokens "source code").  In the case of string literal token concatenation, multiple preprocessing tokens are used to form a single token.  The Token Data for such a token is a space-seperated list of the `preprocessing-token` Token Datas.

Depending on the Token Type the remaining elements are as follows:

#### simple

`simple` represent `keywords`, `operators` and `punctuators`.

This includes `true`, `false`, `nullptr`, `new` and `delete` - for this assignment we will treat them as `keywords` and not `literals` or `operators`.

They are output as

    simple <source> <TERMNAME>

Where `<TERMNAME>` is given in the list below under Terminal Types.

For example, for an input of:

    auto &&

the output is:
 
    simple auto KW_AUTO
    simple && OP_LAND

#### identifier (in output context)

`identifiers` are output as

    identifier <source>
    
Where `<source>` is the identifier in UTF-8, for example for an input of:

    foo
    
the output is:

    identifier foo
        
#### literal

    literal <source> <type> <hexdump>

Where `<type>` is the type of the literal - either a Fundamental Type (defined below), or an `array of <n> <fundamental_type>`.

For example:

    char
    long long int
    array of 42 char16_t

And `<hexdump>` is the hexadecimal representation of the data in memory in the x86-64 ABI (defined below.

For example, for an input of:

    1000000 'A' "ABC" 3.2
    
the output is:

    literal int TODO
    literal char TODO
    literal array of 4 char TODO 00
    literal double TODO

#### user-defined-literal

One of:

    user-defined-literal <source> <ud-suffix> integer <prefix>
    user-defined-literal <source> <ud-suffix> floating <prefix>
    user-defined-literal <source> <ud-suffix> character <type> <hexdump>
    user-defined-literal <source> <ud-suffix> string <type> <hexdump>

Where `<ud-suffix>` is the `ud-suffix`

In the case of `integer` and `floating`, `<prefix>` is the source without the `ud-suffix`

In the case of `string` and `character`, `<type> <hexdump>` have the same meaning as `literal`.

For example:

    123_foo 4.2_bar 0x3_baz
    
outputs:

    user-defined-literal 123_foo _foo integer 123
    user-defined-literal 4.2_bar _bar floating 4.2
    user-defined-literal 0x3_baz _baz integer 0x3

#### invalid

For any valid `preprocessing-token` that does not posttokenize output:

    invalid <source>

For example:

    # 123abc 1..e
    
Should output:

    invalid #
    invalid 123abc
    invalid 1..e

### Features

First you will need to apply the same functionality from pptoken to get the stream of `processing-tokens`.

`whitespace-sequence` and `new-line` are ignored.  (They are for preprocessing, which we shall implement in a later assignment)

Any occurences of `non-whitespace-character` or `header-name` output as an `invalid`.

The remaining `preprocessing-tokens` are:

    identifier
    preprocessing-op-or-punc
    pp-number
    character-literal
    user-defined-character-literal
    string-literal
    user-defined-string-literal

We will discuss each of them:

#### identifier (in preprocessing-token context)

If it is a member of the keyword list, output it as a keyword, otherwise output it as an identifier.

#### preprocessing-op-or-punc

If you encounter the operators `#`, `##`, `%:`, `%:%:` output them as `invalid.

For the remaining `operators` and `punctuation` you should map them to the appropriate `simple-tokens` given in the table below, or output them as `invalid` if not found.

#### pp-number

You need to analyze the string and classify it as matching the `integer-literal`, `floating-literal`, `user-defined-integer-literal` or `user-defined-floating-literal` grammar.

If it doesn't match any of these grammars, output it as `invalid`.

In the case of `user-defined-integer-literal` and `user-defined-floating-literal` you need to split the `ud_suffix` and then output the prefix and `ud-suffix` as a string.

In the case of `integer-literal` and `floating-literal` you will then need to calculate its appropriate type (see 2.14.2 and 2.14.4 in the standard).  If no appropriate type is found output it as an `invalid`.

#### character-literal

Escape sequences (2.14.3.3 Table 7) need to be decoded into their code point.

We shall course define the limits of character literals as follows:

> All character literals must contain one code point in the valid Unicode range is `[0 .. 0xD800)` and `[0xE000 .. 0x110000)`

> If an ordinary character literals code point is less than or equal to 127 (it is in the ASCII range and representable as one UTF-8 code unit), its type is `char`, otherwise its type is `int`.  In both cases its value is its code point.

> For the remaining character literals types (u, U, L) the type is as per the standard (`char16_t`, `char32_t`, `wchar_t`), and ill-formed if the code point doesn't fit in a single code unit.

`L`/`wchar_t` is defined by the x86-64 ABI (defined below) as 32 bits (UTF-32, same as `U`)

#### user-defined-character-literal

The same as for character-literal except extract the `ud-suffix` first

#### string-literal

First decode escape sequences (in non-raw literals) so that each `string-literal` are sequences of code points.

You then need to consider maximal consequtive sequences of both kinds together and apply the phase 6 rules about string concatenation.

A `string-literal` without an encoding prefix is called an _ordinary string literal_.

There are 4 `encoding-prefixes`: `u8` (UTF-8), `u` (UTF-16), `U` (UTF-32), `L` (UTF-32).

We shall course define the rule for concatenating string literals as follows:

> - If a sequence of `string-literals` contains no encoding prefixes, the entire sequence is an treated as an ordinary string literal
> - If a sequence of `string-literals` contains one type of the four `encoding-prefixes`, the entire sequence shall use that prefix.
> - If a sequence of `string-literals` contains two or more different types of the four `encoding-prefixes`, the program is ill-formed.

Read 2.14.5.13 for clarification of this.

Concatenate the sequences and append a terminating 0 code point.

You will then need to UTF-8, UTF-16 or UTF-32 encode them as appropriate depending on the encoding prefix.  As our execution character set is UTF-8, ordinary string literals are UTF-8 (same as `u8`).

The type is `array of <n> <ch>`, where `<n>` is the number of __code units__ (not code points), including terminating 0 code point - and `<ch>` is the appropriate type (`char`, `char16_t`, `char32_t`, `wchar_t`)

#### user-defined-string-literal

The rules are similiar to `string-literal`, and in fact `user-defined-string-literals` and combined with normal `string-literals` as part of the maximal sequence.

The rule about combining `ud-suffixes` is similiar to the one about `encoding-prefixes`.  The total number of `ud-suffix` types must be 0 or 1, or the program is ill-formed.  If it is 0 than it is a normal `string-literal`.  If it is 1 than it is a `user-defined-string-literal` of that type.

Read 2.14.8.8 for clarification.

## Testing / Reference Implementation / Submitting

As per PA1

### Design Notes (Optional)


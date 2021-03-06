#include <unordered_set>
#include <iostream>
#include <vector>
using namespace std;

#include "util/utf8.h"
#include "preprocessor/preprocessor_lexer_error.h"
#include "preprocessor/preprocessor_lexer.h"

// See C++ standard 2.11 Identifiers and Appendix/Annex E.1
const vector<pair<int, int>> identifier_char_allowed_ranges =
{
  {0xA8,0xA8},
  {0xAA,0xAA},
  {0xAD,0xAD},
  {0xAF,0xAF},
  {0xB2,0xB5},
  {0xB7,0xBA},
  {0xBC,0xBE},
  {0xC0,0xD6},
  {0xD8,0xF6},
  {0xF8,0xFF},
  {0x100,0x167F},
  {0x1681,0x180D},
  {0x180F,0x1FFF},
  {0x200B,0x200D},
  {0x202A,0x202E},
  {0x203F,0x2040},
  {0x2054,0x2054},
  {0x2060,0x206F},
  {0x2070,0x218F},
  {0x2460,0x24FF},
  {0x2776,0x2793},
  {0x2C00,0x2DFF},
  {0x2E80,0x2FFF},
  {0x3004,0x3007},
  {0x3021,0x302F},
  {0x3031,0x303F},
  {0x3040,0xD7FF},
  {0xF900,0xFD3D},
  {0xFD40,0xFDCF},
  {0xFDF0,0xFE44},
  {0xFE47,0xFFFD},
  {0x10000,0x1FFFD},
  {0x20000,0x2FFFD},
  {0x30000,0x3FFFD},
  {0x40000,0x4FFFD},
  {0x50000,0x5FFFD},
  {0x60000,0x6FFFD},
  {0x70000,0x7FFFD},
  {0x80000,0x8FFFD},
  {0x90000,0x9FFFD},
  {0xA0000,0xAFFFD},
  {0xB0000,0xBFFFD},
  {0xC0000,0xCFFFD},
  {0xD0000,0xDFFFD},
  {0xE0000,0xEFFFD}
};

// See C++ standard 2.11 Identifiers and Appendix/Annex E.2
const vector<pair<int, int>> identifier_char_disallowed_initially_ranges =
{
  {0x300,0x36F},
  {0x1DC0,0x1DFF},
  {0x20D0,0x20FF},
  {0xFE20,0xFE2F}
};

// See C++ standard 2.13 Operators and punctuators
const unordered_set<string> identifier_like_operators =
{
  "new", "delete", "and", "and_eq", "bitand",
  "bitor", "compl", "not", "not_eq", "or",
  "or_eq", "xor", "xor_eq"
};

/**
 * Determines whether the specified character is a character in the range a-z or A-Z.
 */
bool is_identifier_non_digit(int ch)
{
  switch(ch)
  {
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': case '_':
      return true;

    default:

      //Each universal-character-name in an identifier shall designate a character whose encoding in
      //ISO 10646 falls into one of the ranges specified in E.1.
      for(auto elem : identifier_char_allowed_ranges)
      {
        if(ch >= elem.first
            && ch <= elem.second)
          return true;
      }

      return false;
  }
}

/*
 * Determines whether the specified character is valid for inclusion in an identifier or not
 */
bool valid_identifier_char(int ch)
{
  return is_identifier_non_digit(ch)
         || isdigit(ch);
}

/**
 * Determines whether the specified character is allowed to start an
 * identifier.
 */
bool valid_initial_identifier_char(int ch)
{
  //The initial element shall not be a
  //universal-character-name designating a character whose encoding
  //falls into one of the ranges specified in E.2.
  for(auto elem : identifier_char_disallowed_initially_ranges)
  {
    if(ch >= elem.first
        && ch <= elem.second)
      return false;
  }

  return true;
}

/**
 * Returns the integer value of a hexadecimal digit.
 */
int hex_char_to_int_value(int c)
{
  switch (c)
  {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'A': return 10;
    case 'a': return 10;
    case 'B': return 11;
    case 'b': return 11;
    case 'C': return 12;
    case 'c': return 12;
    case 'D': return 13;
    case 'd': return 13;
    case 'E': return 14;
    case 'e': return 14;
    case 'F': return 15;
    case 'f': return 15;
    default: throw preprocessor_lexer_error("hex_char_to_int_value of nonhex char");
  }
}

/*
 * Folds a trigraph character sequence into it's corresponding output character.
 */
int fold_trigraph(int ch)
{
  switch(ch)
  {
    case '=':
      return '#';

    case '/':
      return '\\';

    case '\'':
      return '^';

    case '(':
      return '[';

    case ')':
      return ']';

    case '!':
      return '|';

    case '<':
      return '{';

    case '>':
      return '}';

    case '-':
      return '~';
  }

  throw preprocessor_lexer_error("Invalid trigraph character sequence.");
}

/**
 * Lexes and appends any user defined string literal suffix.
 */
bool preprocessor_lexer::lex_user_defined_string_literal_suffix(string &lit)
{
  int curr_ch = curr_char();
  bool user_defined_literal = false;

  if(is_identifier_non_digit(curr_ch)
     && valid_initial_identifier_char(curr_ch))
  {
    user_defined_literal = true;
    append_curr_char_to_token_and_advance(lit);

    while(!end_of_buffer()
          && valid_identifier_char(curr_char()))
      append_curr_char_to_token_and_advance(lit);
  }

  return user_defined_literal;
}

/**
 * #include followed by

 * header-name:
 *   < h-char-sequence >
 *   " q-char-sequence "
 * h-char-sequence:
 *   h-char
 *   h-char-sequence h-char
 * h-char:
 *   any member of the source character set except new-line and >
 */
bool preprocessor_lexer::maybe_lex_header_name()
{
  if(valid_identifier_char(curr_char())
     && valid_initial_identifier_char(curr_char()))
  {
    preprocessor_token identifier = lex_identifier();
    mBufferedTokens.push_back(identifier);

    if(identifier.data != "include")
      return false;

    //Skip any whitespace
    if(isspace(curr_char()))
    {
      skip_whitespace();
      mBufferedTokens.push_back(preprocessor_token(PPTOK_WHITESPACE));
    }

    //Save the current position of where we are in case this turns out not to be a header name
    save_current_position();

    if(curr_char() != '<'
        && curr_char() != '"')
    {
      restore_saved_position();
      return false;
    }

    //If we have a <, check it's followed by an identifier
    if(curr_char() == '<')
    {
      int peeked_ch = peek_char();

      if(!valid_identifier_char(peeked_ch))
      {
        restore_saved_position();
        return false;
      }
    }

    int term_ch = curr_char() == '<' ? '>' : '"';
    string header_name;
    append_curr_char_to_token_and_advance(header_name);

    //Lex the h-char-sequence
    while(curr_char() != term_ch)
    {
      append_curr_char_to_token_and_advance(header_name);

      if(curr_char() == '\n')
        throw preprocessor_lexer_error("New line in header name");
    }

    //Append the terminating character
    append_curr_char_to_token_and_advance(header_name);

    discard_saved_position();
    mBufferedTokens.push_back(preprocessor_token(header_name, PPTOK_HEADER_NAME));
  }

  return true;
}

/**
 * Determines if the current character marks the start of an encoding prefix for a
 * string literal.
 */
bool preprocessor_lexer::start_of_encoding_prefix()
{
  bool ret = false;
  int curr_ch = curr_char();

  ++mSuppressTransformations;

  switch(curr_ch)
  {
    case 'u':
    {
      //u, uR, u8, u8R
      if(peek_char() == '\"')
        ret = true;
      else if(peek_char() == '8')
      {
        //Have u8, check for u8R" and u8"
        if(nth_char(2) == '\"'
            || (nth_char(2) == 'R'
                && nth_char(3) == '\"'))
          ret =true;

        break;
      }
      else if(peek_char() == 'R'
              && nth_char(2) == '\"')
        ret = true;

      case 'U':
      case 'L':
      {
        //Check for (L|U)" and (L|U)R"
        if(peek_char() == '\"')
          ret = true;
        else if(nth_char(1) == 'R'
                && nth_char(2) == '\"')
          ret = true;

        break;
      }
    }
  }

  --mSuppressTransformations;
  return ret;
}

/**
 * encoding-prefix:
 *  u8
 *  u8R
 *  u
 *  uR
 *  U
 *  UR
 *  L
 *  LR
 */
void preprocessor_lexer::lex_encoding_prefix(string &prefix)
{
  int curr_ch = curr_char();

  ++mSuppressTransformations;

  switch(curr_ch)
  {
    case 'u':
    {
      //u, uR, u8, u8R
      if(peek_char() == '\"')
        append_curr_char_to_token_and_advance(prefix);
      else if(peek_char() == 'R'
              && nth_char(2) == '\"')
      {
        //Append the uR
        append_chars_to_token_and_advance(prefix, 2);
      }
      else if(peek_char() == '8')
      {
        //Have u8, check for u8R" and u8"
        if(nth_char(2) == '\"')
        {
          //Append the u8
          append_chars_to_token_and_advance(prefix, 2);
        }
        else if(nth_char(2) == 'R'
                && nth_char(3) == '\"')
        {
          //Append the u8R
          append_chars_to_token_and_advance(prefix, 3);
        }
      }

      break;

      case 'U':
      case 'L':
      {
        //U, UR, L, LR
        if(peek_char() == '\"')
        {
          //Append the U or L
          append_curr_char_to_token_and_advance(prefix);
        }
        else if(peek_char() == 'R'
                && nth_char(2) == '\"')
        {
          //UR or LR
          append_chars_to_token_and_advance(prefix, 2);
        }
      }
    }
  }

  --mSuppressTransformations;
}

/**
 * Appends a variable number of characters to the specified token.
 */
void preprocessor_lexer::append_chars_to_token_and_advance(string &tok, int count)
{
  for(int i = 0; i < count; i++)
    append_curr_char_to_token_and_advance(tok);
}

/**
 * raw-string:
 *  " d-char-sequenceopt ( r-char-sequenceopt ) d-char-sequenceopt "
 *
 * r-char-sequence:
 *  r-char
 *  r-char-sequence r-char
 *  r-char:
 *    any member of the source character set, except
 *    a right parenthesis ) followed by the initial d-char-sequence
 *    (which may be empty) followed by a double quote ".
 *
 *  d-char-sequence:
 *    d-char
 *    d-char-sequence d-char
 *
 *  d-char:
 *    any member of the basic source character set except:
 *    space, the left parenthesis (, the right parenthesis ), the backslash \,
 *    and the control characters representing horizontal tab,
 *    vertical tab, form feed, and newline.
 */
void preprocessor_lexer::lex_raw_string_literal_contents(string &literal)
{
  ++mSuppressTransformations;

  //See if the string has a delimiter
  string delimiter;

  while(curr_char() != '(')
  {
    //Check for invalid delimiter characters
    int curr_ch = curr_char();

    if(curr_ch == ' '
        || curr_ch == ')'
        || curr_ch == '\\'
        || curr_ch == '\t'
        || curr_ch == '\v'
        || curr_ch == '\f'
        || curr_ch == '\n')
      throw preprocessor_lexer_error("invalid characters in raw string delimiter");
    else
      append_curr_char_to_token_and_advance(delimiter);
  }

  //Delimiters are limited to 16 chars
  if(delimiter.length() > 16)
    throw preprocessor_lexer_error("raw string delimiters cannot exceed 16 characters");

  //Add the delimiter and opening ( to the literal
  literal += delimiter;
  append_curr_char_to_token_and_advance(literal);

  //add the contents
  while(true)
  {
    //See if this is the terminating d-char-sequence
    if(curr_char() == ')')
    {
      append_curr_char_to_token_and_advance(literal);

      if(delimiter.length() == 0
          && curr_char() == '\"')
      {
        append_curr_char_to_token_and_advance(literal);
        break;
      }
      else if(delimiter.length() > 0
              && match_raw_string_delimiter(delimiter)
              && nth_char(delimiter.length()) == '\"')
      {
        while(curr_char() != '\"')
          append_curr_char_to_token_and_advance(literal);

        append_curr_char_to_token_and_advance(literal);
        break;
      }
    }
    else
      append_curr_char_to_token_and_advance(literal);
  }

  --mSuppressTransformations;
}

/**
 * Determines whether the sequence of chars starting at the current position
 * matches the specified raw string delimiter.
 */
bool preprocessor_lexer::match_raw_string_delimiter(const string &delimiter)
{
  unsigned int remaining_char_count = mBufferEnd - mCurrPosition;

  if(remaining_char_count < delimiter.length())
    return false;

  return string(mCurrPosition, mCurrPosition + delimiter.length()) ==  delimiter;
}

/**
 * Lex's the contents of a string literal. Assumes that the leading prefix or " has
 * already been processed.
 */
void preprocessor_lexer::lex_string_literal_contents(string &literal)
{
  while(curr_char() != '\"')
  {
    if(end_of_buffer())
      throw preprocessor_lexer_error("Unterminated string literal");

    if(curr_char() == '\\')
      append_chars_to_token_and_advance(literal, 2);
    else
      append_curr_char_to_token_and_advance(literal);
  }

  //add the closing "
  append_curr_char_to_token_and_advance(literal);
}

/**
 * Appends the character at the current position to the passed in token data
 * and advances forward one character.
 */
void preprocessor_lexer::append_curr_char_to_token_and_advance(string &tok)
{
  append_char_to_token(curr_char(), tok);
  next_char();
}

/*
 * Appends the specified character to the passed in token, performing
 * any UTF8 encoding/decoding as required.
 */
void preprocessor_lexer::append_char_to_token(int ch, string &tok)
{
  if(ch < 0
     || ch > 127)
  {
    //Have a UTF8 decoded character (> 127) or a UTF8 encoded character
    //that's come from a UCN
    vector<unsigned char> code_units;
    unsigned int encoded_ch = ch < 0 ? ch : encode_to_utf8(ch);

    //Convert the code point into it's code units for adding to the token
    utf8_code_point_to_code_units(encoded_ch, code_units);

    for(auto ch : code_units)
      tok.append(1, (int)ch);
  }
  else if(ch > 127)
  {
    //Decoded UTF8 character
    vector<unsigned char> code_units;
    utf8_code_point_to_code_units(encode_to_utf8(ch), code_units);

    for(auto ch : code_units)
      tok.append(1, (int)ch);
  }
  else
    tok.append(1, ch);
}

/* identifier:
 *   identifier-nondigit
 *   identifier identifier-nondigit
 *   identifier digit
 *
 * identifier-nondigit:
 *   nondigit
 *   universal-character-name
 */
preprocessor_token preprocessor_lexer::lex_identifier()
{
  string identifier;

  while(valid_identifier_char(curr_char()))
  {
    append_curr_char_to_token_and_advance(identifier);

    if(end_of_buffer())
      break;
  }

  //Check whether we have an operator before committing to this being an identifier
  auto itr = identifier_like_operators.find(identifier);

  if(itr == identifier_like_operators.end())
    return preprocessor_token(identifier, PPTOK_IDENTIFIER);
  else
    return preprocessor_token(identifier, PPTOK_PREPROCESSING_OP_OR_PUNC);
}

/**
 * character-literal:
 *   ’ c-char-sequence ’
 *   u’ c-char-sequence ’
 *   U’ c-char-sequence ’
 *   L’ c-char-sequence ’
 *
 * user-defined-character-literal:
 *   character-literal ud-suffix
 */
preprocessor_token preprocessor_lexer::lex_char_literal(bool wide_literal)
{
  string char_lit;
  append_curr_char_to_token_and_advance(char_lit);

  if(wide_literal)
    append_curr_char_to_token_and_advance(char_lit);

  while(true)
  {
    //If this character is a \, skip over the escape character
    if(curr_char() == '\\')
    {
      append_chars_to_token_and_advance(char_lit, 2);
      continue;
    }
    else
      append_curr_char_to_token_and_advance(char_lit);

    //See if we've hit the terminating '
    if(*char_lit.rbegin() == '\'')
      break;
  }

  //If we have the start of an identifier adjacent to the end ", we have a user defined
  //character literal
  bool user_defined_literal = false;

  if(is_identifier_non_digit(curr_char()))
  {
    user_defined_literal = true;
    append_curr_char_to_token_and_advance(char_lit);

    while(!end_of_buffer()
          && valid_identifier_char(curr_char()))
      append_curr_char_to_token_and_advance(char_lit);
  }

  if(user_defined_literal)
    return preprocessor_token(char_lit, PPTOK_USER_DEF_CHAR_LITERAL);
  else
    return preprocessor_token(char_lit, PPTOK_CHAR_LITERAL);
}

/*
 * pp-number:
 *   digit
 *   . digit
 *   pp-number digit
 *   pp-number identifier-nondigit
 *   pp-number e sign
 *   pp-number E sign
 *   pp-number .
 */
void preprocessor_lexer::lex_pp_number(string &num)
{
  int curr_ch = curr_char();

  if(isdigit(curr_ch))
  {
    while(isdigit(curr_char()))
      append_curr_char_to_token_and_advance(num);

    lex_pp_number(num);
  }
  else if(curr_ch == 'e'
          || curr_ch == 'E')
  {
    append_curr_char_to_token_and_advance(num);

    if(curr_char() == '+'
        || curr_char() == '-')
      append_curr_char_to_token_and_advance(num);

    lex_pp_number(num);
  }
  else if(curr_ch == '.'
          || is_identifier_non_digit(curr_ch))
  {
    append_curr_char_to_token_and_advance(num);
    lex_pp_number(num);
  }
}

/*
 * Skips a C++ style comment.
 */
void preprocessor_lexer::skip_cpp_comment()
{
  while(*mCurrPosition != '\n')
  {
    if(end_of_buffer())
      break;

    next_char();
  }
}

/*
 * Skips over a C style comment.
 */
void preprocessor_lexer::skip_c_comment()
{
  while(true)
  {
    if(*mCurrPosition == '*'
        && peek_char() == '/')
    {
      //Consume the '*/' sequence
      skip_chars(2);
      break;
    }
    else
      next_char();

    if(end_of_buffer())
      throw preprocessor_lexer_error("Unexpected end of file found in comment");
  }
}

/*
 * Advances the current character position until a non-whitespace character
 * that is not a new-line is found.
 */
void preprocessor_lexer::skip_whitespace()
{
  while(curr_char() == ' '
        || curr_char() == '\t'
        || curr_char() == '\v'
        || curr_char() == '\f'
        || curr_char() == '\r')
  {
    if(end_of_buffer())
      break;

    //next_char() will handle skipping over adjacent comments
    next_char();
  }
}

/*
 * Returns the next character and advances the current position.
 */
int preprocessor_lexer::next_char()
{
  //Remove any buffered char
  if(!mTransformedChars.empty())
  {
    mTransformedChars.pop_front();

    //If we've still got something buffered, return the first one
    if(!mTransformedChars.empty())
      return mTransformedChars.front();
    else
    {
      //In the case of buffered characters, the current position in the buffer
      //will already be pointing at the first character of the next character
      return curr_char();
    }
  }
  else
  {
    if(mCurrPosition == mBufferEnd)
      return *mBufferEnd;
    else
    {
      ++mCurrPosition;
     return curr_char();
    }
  }
}

/**
 * Accessor for the current character after any transformations have been applied to it.
 */
int preprocessor_lexer::curr_char()
{
  if(!mTransformedChars.empty())
    return mTransformedChars.front();
  else
  {
    int ch = apply_transformations(*mCurrPosition);

    if(!mTransformedChars.empty())
      ch = mTransformedChars.front();

    return ch;
  }
}

/**
 * Accesses the character at the specified distance from the current position.
 */
int preprocessor_lexer::nth_char(unsigned int pos)
{
  if(mTransformedChars.size() > pos)
    return mTransformedChars[pos];
  else
  {
    pos -= mTransformedChars.size();

    if(mCurrPosition + pos > mBufferEnd)
      throw preprocessor_lexer_error("Attempt to access past end of input");

    return *(mCurrPosition + pos);
  }
}

/**
 * Advances the current buffer position by the specified number of characters.
 */
void preprocessor_lexer::skip_chars(unsigned int count)
{
  if(!mTransformedChars.empty())
  {
    for(unsigned int i = 0; i < min(count, (unsigned int)mTransformedChars.size()); i++)
    {
      mTransformedChars.pop_front();
      count -= 1;
    }
  }

  for(unsigned int i = 0; i < count; i++)
  {
    if(end_of_buffer())
      throw preprocessor_lexer_error("Attempt to skip past end of input");
    else
      next_char();
  }
}

/**
 * Applies the transformations specified in phases 1-3:
 * - Encoding of characters to UTF code point.
 * - Universal character names are converted to their corresponding UTF code point
 * - Trigraphs are folded to their corresponding character
 * - Line splicing
 * - Replacement of C/C++ style comments to a single space character
 */
int preprocessor_lexer::apply_transformations(int ch)
{
  //Decode any UTF8 code unit sequences
  if(ch < 0)
  {
    vector<unsigned char> code_units;
    unsigned int num_code_units;
    unsigned char uch = (unsigned char)ch;

    if(uch <= 0xDF)
      num_code_units = 2;
    else if(uch <= 0xEF)
      num_code_units = 3;
    else if(uch <= 0xF7)
      num_code_units = 4;
    else
      throw new preprocessor_lexer_error("Invalid UTF8 character");

    for(unsigned int i = 0; i < num_code_units; i++)
      code_units.push_back(*mCurrPosition++);

    ch = decode_from_utf8(code_units);
    mTransformedChars.push_back(ch);
  }

  if(mSuppressTransformations)
    return ch;

  //Don't want any transformations being applied when peeking/reading the following
  //characters otherwise we could end up in an infinite loop
  ++mSuppressTransformations;

  //Skip any comemnts
  if(ch == '/'
     && peek_char() == '/')
  {
    skip_cpp_comment();
    ch = ' ';
    mTransformedChars.push_back(ch);
  }
  else if(ch == '/'
          && peek_char() == '*')
  {
    skip_c_comment();
    ch = ' ';
    mTransformedChars.push_back(ch);
  }
  else 
  apply_phase_one_transformations(ch);
  apply_phase_two_transformations(ch);

  --mSuppressTransformations;
  return ch;
}

/**
 * Applies the transformations listed in phase 1 in section 2.2.
 */
void preprocessor_lexer::apply_phase_one_transformations(int &ch)
{
  //2.2.1 - Trigraph sequences are replaced by corresponding single-character internal representations.
  if(ch == '?')
  {
    if(!end_of_buffer()
        && peek_char() == '?')
    {
      //We've got '??' which may be the start of a trigraph or simply two ? characters adjacent
      //to each other so peek at the third character to see which one it is.
      int third_ch = nth_char(2);

      if(third_ch == '='
          || third_ch == '/'
          || third_ch == '\''
          || third_ch == ')'
          || third_ch == '('
          || third_ch == '!'
          || third_ch == '<'
          || third_ch == '>'
          || third_ch == '-')
      {
        //Skip the trigraph sequence and fold it to its corresponding character
        skip_chars(3);
        ch = fold_trigraph(third_ch);
        mTransformedChars.push_back(ch);
      }
    }
  }

  if(ch == '\\')
  {
    int peeked_ch = peek_char();
    unsigned int code_unit = 0;

    //Save the current position in case we find that this is not a UCN
    string::const_iterator save_point = mCurrPosition;

    if(peeked_ch == 'u')
    {
      skip_chars(2);

      if(maybe_lex_utf8_code_units(4, code_unit))
      {
        ch = code_unit;
        mTransformedChars.push_back(ch);
      }
      else
        mCurrPosition = save_point;
    }
    else if(peeked_ch == 'U')
    {
      skip_chars(2);

      if(maybe_lex_utf8_code_units(8, code_unit))
      {
        ch = code_unit;
        mTransformedChars.push_back(ch);
      }
      else
        mCurrPosition = save_point;
    }
  }
}

/**
 * Lexes a UTF16 code unit.
 */
bool preprocessor_lexer::maybe_lex_utf16_code_unit(unsigned short &code_unit)
{
  for(int i = 0; i < 4; i++)
  {
    if(end_of_buffer())
    {
      code_unit = 0;
      return false;
    }

    int ch = curr_char();

    if(isxdigit(ch))
    {
      switch(i % 4)
      {
        case 0:
        {
          code_unit = hex_char_to_int_value(ch) << 12;
          break;
        }

        case 1:
        {
          code_unit |= (hex_char_to_int_value(ch) << 8);
          break;
        }

        case 2:
        {
          code_unit |= (hex_char_to_int_value(ch) << 4);
          break;
        }
        case 3:
        {
          code_unit |= hex_char_to_int_value(ch);
          break;
        }
      }
    }
    else
    {
      code_unit = 0;
      return false;
    }

    next_char();
  }

  return true;
}

/**
 * Attempts to lex a specified number of UTF-8 code units represented as
 * hexadecimal quads where a hex-quad is defined in 2.3.2 as:
 *
 * hex-quad:
 *   hexadecimal-digit hexadecimal-digit hexadecimal-digit hexadecimal-digit
 *
 * Returns true if the required number of code units was lexed, otherwise false.
 */
bool preprocessor_lexer::maybe_lex_utf8_code_units(unsigned int num_code_units, unsigned int &code_point)
{
  for(unsigned int i = num_code_units / 4; i > 0; i--)
  {
    unsigned short utf16_unit = 0;

    if(maybe_lex_utf16_code_unit(utf16_unit))
      code_point |= utf16_unit << ((i - 1) * 16);
    else
    {
      code_point = 0;
      return false;
    }

  }

  return true;
}

/**
 * Applies the transformations listed in phase 2 in section 2.2.
 */
void preprocessor_lexer::apply_phase_two_transformations(int &ch)
{
  //2.2.1 - Each instance of a backslash character (\) immediately followed by a new-line
  //character is deleted.
  if(ch == '\\')
  {
    if(!end_of_buffer()
        && peek_char() == '\n')
    {
      //Skip over the \ and new-line. If we hit the end of the input then clamp
      //the current char to the final new line
      skip_chars(2);

      if(end_of_buffer())
        ch = *mBufferEnd;
      else
        ch = curr_char();
    }
  }
}

/**
* Scans the next token or sequence of tokens.
*/
void preprocessor_lexer::scan_next_token()
{
  if(!end_of_buffer())
  {
    bool header_name_allowed = mLastChar == -1 || mLastChar == '\n';
    int curr_ch = curr_char();
    mLastChar = curr_ch;

    switch(curr_ch)
    {
      case '#':
      {
        string tok;
        tok.append(1, curr_ch);

        int peeked_ch = peek_char();

        if(peeked_ch == '#')
        {
          skip_chars(2);
          tok.append(1, peeked_ch);

          mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
        }
        else
        {
          //Buffer the # token
          next_char();

          preprocessor_token hash_tok({(char)curr_ch}, PPTOK_PREPROCESSING_OP_OR_PUNC);
          mBufferedTokens.push_back(hash_tok);

          if(header_name_allowed)
            maybe_lex_header_name();
        }

        break;
      }

      case '<':
      {
        string tok;
        append_curr_char_to_token_and_advance(tok);

        //May be <, <<, <<=, <=, <: or <%
        if(curr_char() == ':')
        {
          //<::: is tokenised as '<:' and '::'
          //<::> is tokenised as '<:' and ':>'
          //<::! is tokenised as '<', '::' and '!'
          //<:   is tokenised as '<:'
          if(peek_char() != ':')
          {
            append_curr_char_to_token_and_advance(tok);
            mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
          }
          else
          {
            //We have <::
            //2.5.3 - Otherwise, if the next three characters are <:: and the subsequent character is
            //neither : nor >, the < is treated as a preprocessor token by itself and not as the first
            //character of the alternative token <:
            if(nth_char(2) != ':'
                && nth_char(2) != '>')
            {
              //The < needs to be treated as a separate pre-processing token
              //and not the start of a <: alternate token
              mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
            }
            else
            {
              //Append the : to form a <: token
              append_curr_char_to_token_and_advance(tok);
              mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
            }

            //Consume and output the ::
            tok = "";
            append_chars_to_token_and_advance(tok, 2);
            mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
          }

          break;

        }
        else if(curr_char() == '%'
                || curr_char() == '=')
          append_curr_char_to_token_and_advance(tok);
        else if(curr_char() == '<')
        {
          append_curr_char_to_token_and_advance(tok);

          if(curr_char() == '=')
            append_curr_char_to_token_and_advance(tok);
        }

        mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
        break;
      }

      case '>':
      {
        string tok;
        append_curr_char_to_token_and_advance(tok);

        //May have >, >>, >>=, or >=
        if(curr_char() == '>')
        {
          append_curr_char_to_token_and_advance(tok);

          if(curr_char() == '=')
            append_curr_char_to_token_and_advance(tok);
        }
        else if(curr_char() == '=')
          append_curr_char_to_token_and_advance(tok);

        mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
        break;
      }

      case '%':
      {
        string tok;
        append_curr_char_to_token_and_advance(tok);

        if(curr_char() == ':')
        {
          append_curr_char_to_token_and_advance(tok);

          if(curr_char() == '%'
              && peek_char() == ':')
            append_chars_to_token_and_advance(tok, 2);
          else if(header_name_allowed)
          {
            mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
            maybe_lex_header_name();
            break;
          }
        }
        else if(curr_char() == '>'
                || curr_char() == '=')
          append_curr_char_to_token_and_advance(tok);

        mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
        break;
      }

      case ':':
      {
        string tok;
        append_curr_char_to_token_and_advance(tok);

        if(curr_char() == '>'
            || curr_char() == ':')
          append_curr_char_to_token_and_advance(tok);

        mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
        break;
      }

      case '|':
      case '&':
      {
        //Maybe a op, op op,  or op=
        string tok;
        append_curr_char_to_token_and_advance(tok);

        if(curr_char() == curr_ch
            || curr_char() == '=')
          append_curr_char_to_token_and_advance(tok);

        mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
        break;
      }

      case '{':
      case '}':
      case '[':
      case ']':
      case '(':
      case ')':
      case ';':
      case '?':
      case '~':
      case ',':
      {
        string tok;
        append_curr_char_to_token_and_advance(tok);
        mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
        break;
      }

      case '^':
      case '/':
      case '*':
      {
        string tok;
        append_curr_char_to_token_and_advance(tok);

        //May have op or op=
        if(curr_char() == '=')
          append_curr_char_to_token_and_advance(tok);

        mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
        break;
      }

      case '+':
      {
        string tok;
        append_curr_char_to_token_and_advance(tok);

        //May have +, ++ or +=
        if(curr_char() == '+'
            || curr_char() == '=')
          append_curr_char_to_token_and_advance(tok);

         mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
         break;
      }

      case '-':
      {
        string tok;
        append_curr_char_to_token_and_advance(tok);

        //May have -, --, -=, -> or ->*
        if(curr_char() == '-'
            || curr_char() == '=')
          append_curr_char_to_token_and_advance(tok);
        else if(curr_char() == '>')
        {
          append_curr_char_to_token_and_advance(tok);

          if(curr_char() == '*')
            append_curr_char_to_token_and_advance(tok);
        }

        mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
        break;
      }

      case ' ':
      case '\t':
      case '\v':
      case '\f':
      case '\r':
      {
        skip_whitespace();
        mBufferedTokens.push_back(preprocessor_token(PPTOK_WHITESPACE));
        break;
      }

      case '\n':
      {
        //Only want to skip over the new-line char
        ++mCurrPosition;
        mBufferedTokens.push_back(preprocessor_token(PPTOK_NEW_LINE));
        break;
      }

      case '\"':
      {
        string lit;

        append_curr_char_to_token_and_advance(lit);
        lex_string_literal_contents(lit);

        if(lex_user_defined_string_literal_suffix(lit))
          mBufferedTokens.push_back(preprocessor_token(lit, PPTOK_USER_DEF_STRING_LITERAL));
        else
          mBufferedTokens.push_back(preprocessor_token(lit, PPTOK_STRING_LITERAL));

        break;
      }

      case '\'':
      {
        mBufferedTokens.push_back(lex_char_literal(/*wide_literal=*/false));
        break;
      }
	  
      case 'R':
      {
        if(peek_char() == '\"')
        {
          //Raw string
          string lit;

          append_chars_to_token_and_advance(lit, 2);
          lex_raw_string_literal_contents(lit);

          mBufferedTokens.push_back(preprocessor_token(lit, PPTOK_STRING_LITERAL));
        }
        else
        {   
          //TODO: this looks wrong
          string identifier;
          mBufferedTokens.push_back(preprocessor_token(identifier, PPTOK_IDENTIFIER));
        }

        break;
      }

      case 'U':
      case 'u':
      case 'L':
      {
        if(peek_char() == '\'')
        {
          mBufferedTokens.push_back(lex_char_literal(/*wide_literal=*/true));
          break;
        }
        else if(start_of_encoding_prefix())
        {
          string prefix = "";
          lex_encoding_prefix(prefix);

          //Add the opening " and the string contents
          append_curr_char_to_token_and_advance(prefix);

          if(prefix[prefix.length() - 2] == 'R')
            lex_raw_string_literal_contents(prefix);
          else
            lex_string_literal_contents(prefix);

          if(lex_user_defined_string_literal_suffix(prefix))
            mBufferedTokens.push_back(preprocessor_token(prefix, PPTOK_USER_DEF_STRING_LITERAL));
          else
            mBufferedTokens.push_back(preprocessor_token(prefix, PPTOK_STRING_LITERAL));

          break;
        }

        //Fallthru and treat the U, u, L or R as an identifier
      }

      case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
      case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
      case 's': case 't': case 'v': case 'w': case 'x': case 'y': case 'z':
      case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
      case 'J': case 'K': case 'M': case 'N': case 'O': case 'P': case 'Q':
      case 'S': case 'T': case 'V': case 'W': case 'X': case 'Y': case 'Z': case '_':
      {
        mBufferedTokens.push_back(lex_identifier());
        break;
      }

      case '.':
      {
        if(nth_char(1) == '.'
            && nth_char(2) == '.')
        {
          //We have ...
          string tok;

          append_chars_to_token_and_advance(tok, 3);
          mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
          break;
        }

        //Fallthru for single '.' case
      }

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      {
        int peeked_ch = peek_char();
        string tok;

        if(curr_ch == '.'
            && !isdigit(peeked_ch))
        {
          append_curr_char_to_token_and_advance(tok);

          if(curr_char() == '*')
            append_curr_char_to_token_and_advance(tok);

          mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
        }
        else
        {
          string num;
          append_curr_char_to_token_and_advance(num);

          preprocessor_token num_tok(num, PPTOK_NUMBER);
          lex_pp_number(num_tok.data);
          mBufferedTokens.push_back(num_tok);
        }

        break;
      }

      case '!':
      {
        string tok;
        append_curr_char_to_token_and_advance(tok);

        if(curr_char() == '=')
          append_curr_char_to_token_and_advance(tok);

        mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
        break;
      }

      case '=':
      {
        string tok;
        append_curr_char_to_token_and_advance(tok);

        if(curr_char() == '=')
          append_curr_char_to_token_and_advance(tok);

        mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_PREPROCESSING_OP_OR_PUNC));
        break;
      }

      case '\\':
      {
        //Consume the '\'
        next_char();

        //If the next character is a new-line, consume the newline and carry on with the next character
        if(!end_of_buffer()
            && curr_char() == '\n')
          next_char();
        else
        {
          //Treat it as a non-whitespace char
          string non_ws_char;
          non_ws_char.append(1, curr_ch);
          mBufferedTokens.push_back(preprocessor_token(non_ws_char, PPTOK_NON_WHITESPACE_CHAR));
        }

        break;
      }

      default:

        string tok;

        if(curr_ch >= 0
            && curr_ch <= 127)
          append_char_to_token(curr_ch, tok);
        else
        {
          //UCNs may start an identifier.
          if(valid_identifier_char(curr_ch)
              && valid_initial_identifier_char(curr_ch))
          {
            mBufferedTokens.push_back(lex_identifier());
            break;
          }
          else
            append_char_to_token(curr_ch, tok);
        }

        mBufferedTokens.push_back(preprocessor_token(tok, PPTOK_NON_WHITESPACE_CHAR));

        if(!end_of_buffer())
          next_char();
    }
  }
  else if(!mEndOfFileTokensProcessed)
	{
    //If the input is not empty and does not end in a new-line, insert one
    if(mBuffer.length() > 0
       && mLastChar != '\n')
      mBufferedTokens.push_back(preprocessor_token(PPTOK_NEW_LINE));

    mBufferedTokens.push_back(preprocessor_token(PPTOK_EOF));
    mEndOfFileTokensProcessed = true;
	}
}

preprocessor_token preprocessor_lexer::next_token()
{
  scan_next_token();

  preprocessor_token tok = mBufferedTokens.front();
  mBufferedTokens.pop_front();

  return tok;
 }
 
bool preprocessor_lexer::finished_tokenising()
{
  if(!mBufferedTokens.empty())
    return false;

  return mCurrPosition == mBufferEnd
         && mTransformedChars.empty()
         && mEndOfFileTokensProcessed;
}

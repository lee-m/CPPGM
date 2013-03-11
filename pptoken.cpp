#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cstring>
using namespace std;

#include "IPPTokenStream.h"
#include "DebugPPTokenStream.h"

// Translation features you need to implement:
// - utf8 decoder
// - utf8 encoder
// - universal-character-name decoder

// given hex digit character c, return its value
int HexCharToValue(int c)
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
  default: throw logic_error("HexCharToValue of nonhex char");
  }
}

// See C++ standard 2.11 Identifiers and Appendix/Annex E.1
const vector<pair<int, int>> AnnexE1_Allowed_RangesSorted =
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
const vector<pair<int, int>> AnnexE2_DisallowedInitially_RangesSorted =
{
  {0x300,0x36F},
  {0x1DC0,0x1DFF},
  {0x20D0,0x20FF},
  {0xFE20,0xFE2F}
};

// See C++ standard 2.13 Operators and punctuators
const unordered_set<string> Digraph_IdentifierLike_Operators =
{
  "new", "delete", "and", "and_eq", "bitand",
  "bitor", "compl", "not", "not_eq", "or",
  "or_eq", "xor", "xor_eq"
};

// See `simple-escape-sequence` grammar
const unordered_set<int> SimpleEscapeSequence_CodePoints =
{
  '\'', '"', '?', '\\', 'a', 'b', 'f', 'n', 'r', 't', 'v'
};

class PPTokeniserException : public exception
{
private:
  string mMessage;

public:
  PPTokeniserException(const string &message) : mMessage(message) {}
  ~PPTokeniserException() throw() {}
  const char* what() const throw() { return mMessage.c_str(); }
};

// Tokenizer
class PPTokeniser
{
private:
  IPPTokenStream& mOutput;

  //Pointers to the current input buffer, the end of the input buffer and the current position within the buffer
  char *mBuffer;
  char *mBufferEnd;
  char *mCurrPosition;
  char mLastChar;

  //Whether to suppress the standard transformations to apply when lexing each char
  int mSuppressTransformations;

public:
  PPTokeniser(IPPTokenStream& output, const string &input)
    : mOutput(output)
  {
    //Copy the input into a writable buffer so that trigraphs and UCNs can be folded in-place
    mBuffer = new char[input.length() + 1];
    memcpy(mBuffer, input.c_str(), input.length());

    mBufferEnd = mBuffer + input.length();
    mCurrPosition = mBuffer;
    mSuppressTransformations = 0;
  }
   
  ~PPTokeniser()
  {
    delete [] mBuffer;
    mBuffer = 0;
    mBufferEnd = 0;
    mCurrPosition = 0;
  }

  void process()
  {  
    skip_whitespace();

    while(!end_of_input())
      {
        int curr_ch = curr_char();
        mLastChar = curr_ch;

        switch(curr_ch)
          {
          case '#':
            {
              //TODO: handle includes
              string tok;
              tok.append(1, curr_ch);

              int peeked_ch = peek_char();

              if(peeked_ch == '#')
                {
                  skip_chars(2);
                  tok.append(1, peeked_ch);
                }
              else
                next_char();

              mOutput.emit_preprocessing_op_or_punc(tok);
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
                      mOutput.emit_preprocessing_op_or_punc(tok);
                    }
                  else
                    {
                      //We have <:: 
                      //2.5.3 - Otherwise, if the next three characters are <:: and the subsequent character is 
                      //neither : nor >, the < is treated as a preprocessor token by itself and not as the first 
                      //character of the alternative token <:
                      if(nth_char(2) != ':' && nth_char(2) != '>')
                        {
                          //The < needs to be treated as a separate pre-processing token
                          //and not the start of a <: alternate token
                          mOutput.emit_preprocessing_op_or_punc(tok);
                        }
                      else
                        {
                          //Append the : to form a <: token
                          append_curr_char_to_token_and_advance(tok);
                          mOutput.emit_preprocessing_op_or_punc(tok);   
                        }

                      //Consume and output the ::
                      tok = "";
                      append_chars_to_token_and_advance(tok, 2);
                      mOutput.emit_preprocessing_op_or_punc(tok);
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

              mOutput.emit_preprocessing_op_or_punc(tok);
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

              mOutput.emit_preprocessing_op_or_punc(tok);
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
                }
              else if(curr_char() == '>' 
                      || curr_char() == '=')
                append_curr_char_to_token_and_advance(tok);

              mOutput.emit_preprocessing_op_or_punc(tok);
              break;
            }

          case ':':
            {
              string tok;
              append_curr_char_to_token_and_advance(tok);

              if(curr_char() == '>' 
                 || curr_char() == ':')
                append_curr_char_to_token_and_advance(tok);

              mOutput.emit_preprocessing_op_or_punc(tok);
              break;
            }

          case '|': case '&':
            {
              //Maybe a op, op op,  or op=
              string tok;
              append_curr_char_to_token_and_advance(tok);

              if(curr_char() == curr_ch
                 || curr_char() == '=')
                append_curr_char_to_token_and_advance(tok);
              
              mOutput.emit_preprocessing_op_or_punc(tok);
              break;
            }

          case '{': case '}': case '[': case ']': case '(': case ')': case ';': case '?':
          case '~': case ',':
            {
              string tok;
              append_curr_char_to_token_and_advance(tok);
              mOutput.emit_preprocessing_op_or_punc(tok);
              break;
            }
              
          case '^': case '/': case '*':
            {
              string tok;
              append_curr_char_to_token_and_advance(tok);
              
              //May have op or op=
              if(curr_char() == '=')
                append_curr_char_to_token_and_advance(tok);
              
              mOutput.emit_preprocessing_op_or_punc(tok);
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

               mOutput.emit_preprocessing_op_or_punc(tok);
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

              mOutput.emit_preprocessing_op_or_punc(tok);
              break;
            }

          case ' ': case '\t': case '\v': case '\f': case '\r':
            {
              skip_whitespace();
              mOutput.emit_whitespace_sequence();
              break;
            }

          case '\n':
            {
              //Only want to skip over the new-line char
              ++mCurrPosition;
              mOutput.emit_new_line();
              break;
            }
            
          case '\"':
            {
              string lit;
              bool user_defined_literal = false;

              append_curr_char_to_token_and_advance(lit);
              lex_string_literal_contents(lit);
           
              //If we have the start of an identifier adjacent to the end ", we have a user defined 
              //string literal
              if(is_identifier_non_digit(curr_char()))
                {
                  user_defined_literal = true;
                  append_curr_char_to_token_and_advance(lit);

                  while(!end_of_input()
                        && valid_identifier_char(curr_char()))
                    append_curr_char_to_token_and_advance(lit);
                }

              if(user_defined_literal)
                mOutput.emit_user_defined_string_literal(lit);
              else
                mOutput.emit_string_literal(lit);
              break;
            }

          case '\'':
            {
              lex_char_literal(/*wide_literal=*/false);
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

                  mOutput.emit_string_literal(lit);
                }
              else
                lex_identifier();
                  
              break;
            }

          case 'U': case 'u': case 'L':
            {
              if(peek_char() == '\'')
                {
                  lex_char_literal(/*wide_literal=*/true);
                  break;
                }
              else if(start_of_encoding_prefix())
                {
                  string prefix = "";
                  lex_encoding_prefix(prefix);

                  //Add the opening " and the string contents
                  append_curr_char_to_token_and_advance(prefix);

                  if(prefix[prefix.length() -2] == 'R')
                    lex_raw_string_literal_contents(prefix);
                  else
                    lex_string_literal_contents(prefix);

                  mOutput.emit_string_literal(prefix);
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
              lex_identifier();
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
                  mOutput.emit_preprocessing_op_or_punc(tok);
                  break;
                }

                //Fallthru for single '.' case
            }                                      
          case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': 
            {
              int peeked_ch = peek_char();
              string tok;

              if(curr_ch == '.'
                 && !isdigit(peeked_ch))
                {
                  append_curr_char_to_token_and_advance(tok);

                  if(curr_char() == '*')
                    append_curr_char_to_token_and_advance(tok);

                  mOutput.emit_preprocessing_op_or_punc(tok);
                }
              else
                {
                  string num;
                  append_curr_char_to_token_and_advance(num);

                  lex_pp_number(num);
                  mOutput.emit_pp_number(num);
                }

              break;
            }

          case '!':
            {
              string tok;
              append_curr_char_to_token_and_advance(tok);

              if(curr_char() == '=')
                append_curr_char_to_token_and_advance(tok);

              mOutput.emit_preprocessing_op_or_punc(tok);
              break;
            }

          case '=':
            {
              string tok;
              append_curr_char_to_token_and_advance(tok);

              if(curr_char() == '=')
                append_curr_char_to_token_and_advance(tok);

              mOutput.emit_preprocessing_op_or_punc(tok);
              break;
            }

          case '\\':
            {
              //Consume the '\'
              next_char();

              //If the next character is a new-line, consume the newline and carry on with the next character
              if(!end_of_input() 
                 && curr_char() == '\n')
                next_char();
              else
                {
                  //Treat it as a non-whitespace char
                  string non_ws_char;
                  non_ws_char.append(1, curr_ch);
                  mOutput.emit_non_whitespace_char(non_ws_char);
                }

              break;
            }

          default:

            string tok;
            tok.append(1, curr_ch);
            mOutput.emit_non_whitespace_char(tok);

            if(!end_of_input())
              next_char();
          }
      }

    //If the input is not empty and does not end in a new-line, insert one
    if(mBuffer != mBufferEnd 
       && *(mBufferEnd - 1) != '\n')
      mOutput.emit_new_line();

    mOutput.emit_eof();
  }

  /**
   * Determines if the current character marks the start of an encoding prefix for a
   * string literal.
   */
  bool start_of_encoding_prefix()
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

        case 'U': case 'L':
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
  void lex_encoding_prefix(string &prefix)
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

        case 'U': case 'L':
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
  void append_chars_to_token_and_advance(string &tok, int count)
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
  void lex_raw_string_literal_contents(string &literal)
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
          throw PPTokeniserException("invalid characters in raw string delimiter");
        else
          append_curr_char_to_token_and_advance(delimiter);
      }

    //Delimiters are limited to 16 chars
    if(delimiter.length() > 16)
      throw PPTokeniserException("raw string delimiters cannot exceed 16 characters");

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

            if(delimiter.length() == 0 && curr_char() == '\"')
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
  bool match_raw_string_delimiter(const string &delimiter)
  {
    unsigned int remaining_char_count = mBufferEnd - mCurrPosition;

    if(remaining_char_count < delimiter.length())
      return false;

    return strncmp(mCurrPosition, delimiter.c_str(), delimiter.length()) == 0;
  }
  
  /**
   * Lex's the contents of a string literal. Assumes that the leading prefix or " has
   * already been processed.
   */
  void lex_string_literal_contents(string &literal)
  {
    while(true)
      {
        if(end_of_input())
          throw PPTokeniserException("Unterminated string literal");

        int ch = curr_char();
        append_curr_char_to_token_and_advance(literal);

        if(ch == '\"')
          break;
      }
  }

  /**
   * Appends the character at the current position to the passed in token data
   * and advances forward one character.
   */
  void append_curr_char_to_token_and_advance(string &tok)
  {
    tok.append(1, (char)curr_char());
    next_char();
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
  void lex_identifier()
  {
    string identifier;
    int ch = *mCurrPosition;

    while(valid_identifier_char(ch))
      {
        identifier += *mCurrPosition;
        ch = next_char();

        if(end_of_input())
          break;
      }
    
    //Check whether we have an operator before committing to this being an identifier
    auto itr = Digraph_IdentifierLike_Operators.find(identifier);

    if(itr == Digraph_IdentifierLike_Operators.end())
      mOutput.emit_identifier(identifier);
    else
      mOutput.emit_preprocessing_op_or_punc(identifier);
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
  void lex_char_literal(bool wide_literal)
  {
    string char_lit;
    append_curr_char_to_token_and_advance(char_lit);

    if(wide_literal)
      append_curr_char_to_token_and_advance(char_lit);

    while(curr_char() != '\'')
      {
        if(end_of_input())
          throw PPTokeniserException("Unterminated character literal");
        else
          {
            append_curr_char_to_token_and_advance(char_lit);

            if(curr_char() == '\'')
              {
                append_curr_char_to_token_and_advance(char_lit);
                break;
              }
          }
      }

    //If we have the start of an identifier adjacent to the end ", we have a user defined 
    //string literal
    bool user_defined_literal = false;

    if(is_identifier_non_digit(curr_char()))
      {
        user_defined_literal = true;
        append_curr_char_to_token_and_advance(char_lit);

        while(!end_of_input()
              && valid_identifier_char(curr_char()))
          append_curr_char_to_token_and_advance(char_lit);
      }

    if(user_defined_literal)
      mOutput.emit_user_defined_character_literal(char_lit);
    else
      mOutput.emit_character_literal(char_lit);
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
  void lex_pp_number(string &num)
  {  
    int curr_ch = curr_char();

    if(isdigit(curr_ch))
      {
        while(isdigit(curr_char()))
          append_curr_char_to_token_and_advance(num);

        lex_pp_number(num);
      }
    else if(curr_ch == 'e' || curr_ch == 'E')
      {
        append_curr_char_to_token_and_advance(num);

        if(curr_char() == '+' || curr_char() == '-')
          append_curr_char_to_token_and_advance(num);

        lex_pp_number(num);
      }
    else if(curr_ch == '.' || is_identifier_non_digit(curr_ch))
      {
        append_curr_char_to_token_and_advance(num);
        lex_pp_number(num);
      }
  }

  /*
   * Skips a C++ style comment.
   */
  void skip_cpp_comment()
  {
    while(*mCurrPosition != '\n')
      next_char();
  }

  /*
   * Skips over a C style comment.
   */
  void skip_c_comment()
  {
    while(true)
      {
        if(*mCurrPosition == '*' && peek_char() == '/')
          {
            //Consume the '*/' sequence
            next_char();
            next_char();
            break;
          }
        else
          next_char();

        if(end_of_input())
          throw PPTokeniserException("Unexpected end of file found in comment");
      }
  }

  /*
   * Determines whether the specified character is valid for inclusion in an identifier or not
   */
  bool valid_identifier_char(int ch)
  {
    return is_identifier_non_digit(ch) || isdigit(ch);
  }
  
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

        //TODO: UCNs are valid identifier-non-digit. 
        //Each universal-character-name in an identifier shall designate a character whose encoding in 
        //ISO 10646 falls into one of the ranges specified in E.1. The initial element shall not be a 
        //universal-character-name designating a character whose encoding
        //falls into one of the ranges specified in E.2.
        return false;
      }
  }

  /*
   * Advances the current character position until a non-whitespace character 
   * that is not a new-line is found.
   */
  void skip_whitespace()
  {
    while(*mCurrPosition == ' '
          || *mCurrPosition == '\t'
          || *mCurrPosition == '\v'
          || *mCurrPosition == '\f'
          || *mCurrPosition == '\r')
      {
        //next_char() will handle skipping over adjacent comments
        next_char();
      }
  }

  /*
   * Indicates whether the end of the input has been reached.
   */
  bool end_of_input()
  {
    return mCurrPosition == mBufferEnd;
  }

  /*
   * Returns the next character in the stream without advancing the current position.
   */
  int peek_char()
  {
    return nth_char(1);
  }

  /*
   * Returns the next character and advances the current position.
   */
  int next_char()
  {
    if(mCurrPosition == mBufferEnd)
      throw PPTokeniserException("Unexpected end of file found.");

    ++mCurrPosition;
    return curr_char();
  }

  /**
   * Accessor for the current character after any transformations have been applied to it.
   */
  int curr_char()
  {
    return apply_transformations(*mCurrPosition);
  }

  /**
   * Accesses the character at the specified distance from the current position.
   */
  int nth_char(unsigned int pos)
  {
    if(mCurrPosition + pos > mBufferEnd)
      throw PPTokeniserException("Attempt to access past end of input");

    return *(mCurrPosition + pos);
  }

  /** 
   * Advances the current buffer position by the specified number of characters.
   */
  void skip_chars(unsigned int count)
  {
    if(mCurrPosition + count > mBufferEnd)
      throw PPTokeniserException("Attempt to skip past end of input");

    mCurrPosition += count;
  }

  /**
   * Applies the transformations specified in phases 1-3:
   * - Encoding of characters to UTF code point.
   * - Universal character names are converted to their corresponding UTF code point
   * - Trigraphs are folded to their corresponding character
   * - Line splicing
   * - Replacement of C/C++ style comments to a single space character
   */
  int apply_transformations(int ch)
  {
    if(mSuppressTransformations)
      return ch;

    //Don't want any transformations being applied when peeking/reading the following
    //characters otherwise we could end up in an infinite loop
    ++mSuppressTransformations;

    //Skip any comemnts
    if(ch == '/' && peek_char() == '/')
      {
        skip_cpp_comment();
        ch = ' ';
      }
    else if(ch == '/' && peek_char() == '*')
      {
        skip_c_comment();
        ch = ' ';
      }

    apply_phase_one_transformations(ch);
    apply_phase_two_transformations(ch);
    
    --mSuppressTransformations;
    return ch;
  }

  /** 
   * Applies the transformations listed in phase 1 in section 2.2.
   */
  void apply_phase_one_transformations(int &ch)
  {
    //2.2.1 - Trigraph sequences are replaced by corresponding single-character internal representations.
    if(ch == '?')
      {
        if(!end_of_input() 
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
                //Skip the '??' and fold the trigraph to its corresponding character
                skip_chars(2);
                ch = fold_trigraph(curr_char());

                //Replace the final trigraph char with its folded equivalent
                *mCurrPosition = ch;
              }           
          }
      }

    //TODO: UCN decoding
  }

  /** 
   * Applies the transformations listed in phase 2 in section 2.2.
   */
  void apply_phase_two_transformations(int &ch)
  {
    //2.2.1 - Each instance of a backslash character (\) immediately followed by a new-line 
    //character is deleted.
    if(ch == '\\')
      {
        if(!end_of_input() 
           && peek_char() == '\n')
          {
            //Skip over the \ and new-line. If we hit the end of the input then clamp
            //the current char to the final new line
            skip_chars(2);
            
            if(end_of_input())
              ch = *mBufferEnd;
            else
              ch = curr_char();
          }
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

    throw PPTokeniserException("Invalid trigraph character sequence.");
  }
};

int main()
{
  try
  {
    ostringstream oss;
    oss << cin.rdbuf();

    string input = oss.str();

    DebugPPTokenStream output;
    PPTokeniser tokenizer(output, input);

    tokenizer.process();
  }
  catch (exception& e)
  {
    cerr << "ERROR: " << e.what() << endl;
    return EXIT_FAILURE;
  }
  catch(...)
    {
      cerr << "Exception" << endl;
      return EXIT_FAILURE;
    }
}


#ifndef PREPROCESSOR_LEXER_H
#define PREPROCESSOR_LEXER_H

#include <string>
#include <deque>
using std::string;
using std::deque;

//Different types of preprocessing tokens
enum preprocessor_token_type
{
  PPTOK_WHITESPACE = 0,
  PPTOK_NEW_LINE,
  PPTOK_HEADER_NAME,
  PPTOK_IDENTIFIER,
  PPTOK_NUMBER,
  PPTOK_CHAR_LITERAL,
  PPTOK_USER_DEF_CHAR_LITERAL,
  PPTOK_STRING_LITERAL,
  PPTOK_USER_DEF_STRING_LITERAL,
  PPTOK_PREPROCESSING_OP_OR_PUNC,
  PPTOK_NON_WHITESPACE_CHAR,
  PPTOK_EOF
};

//A single preprocessing token containing its data and its type.
struct preprocessor_token
{
  preprocessor_token(preprocessor_token_type tok_type) : type(tok_type) {}
  preprocessor_token(const string &tok_data, preprocessor_token_type tok_type) : data(tok_data), type(tok_type) { }
  
  //Contents of the token
  string data;

  //Type of the token
  preprocessor_token_type type;
};

//Lexer which tokenises input source code into a series of preprocessor tokens.
class preprocessor_lexer
{
private:

  //The current input buffer, the end of the input buffer and the current position within the buffer
  string mBuffer;
  string::const_iterator mBufferEnd;
  string::const_iterator mCurrPosition;

  //Last character that was processed.
  int mLastChar;

  //Whether to suppress the standard transformations to apply when lexing each char
  int mSuppressTransformations;

  //Buffer containing any characters produced as a result of UCN/trigraph
  //transformations etc..
  deque<int> mTransformedChars;

  //In some cases, calling next_token may result in one or more tokens actually being
  //produced to resolve the ambiguity about what token the current position is referring
  //to. This queue stores any pre-lexed tokens which should be returned on the next call
  //to next_token
  deque<preprocessor_token> mBufferedTokens;

  //Saved current position/transformed chars to allow the current position
  //to be rewound at a later point.
  string::const_iterator mSavedCurrPosition;
  deque<int> mSavedTransformedChars;

  //If true, any synthesised EOF/new line tokens have already been added 
  bool mEndOfFileTokensProcessed;

  //Methods to handle lexing of particular tokens
  bool lex_user_defined_string_literal_suffix(string &lit);
  void lex_encoding_prefix(string &prefix);
  void lex_raw_string_literal_contents(string &literal);
  void lex_string_literal_contents(string &literal);
  preprocessor_token lex_char_literal(bool wide_literal);
  preprocessor_token lex_identifier();
  void lex_pp_number(string &num);

  //Methods to attempt lexing of a particular token type.
  bool maybe_lex_header_name();
  bool maybe_lex_utf16_code_unit(unsigned short &code_unit);
  bool maybe_lex_utf8_code_units(unsigned int num_code_units, unsigned int &code_point);

  //Miscalleneous helper methods
  bool start_of_encoding_prefix();
  bool match_raw_string_delimiter(const string &delimiter);

  //Helper methods to append a character to a current token
  void append_chars_to_token_and_advance(string &tok, int count);
  void append_curr_char_to_token_and_advance(string &tok);
  void append_char_to_token(int ch, string &tok);

  //Methods to skip over various parts of the input
  void skip_cpp_comment();
  void skip_c_comment();
  void skip_whitespace();
  void skip_chars(unsigned int count);

  //Methods to advance/get the current position
  int next_char();
  int curr_char();
  int nth_char(unsigned int pos);
  
  //Method to apply the required transformations for a particular input character.
  int apply_transformations(int ch);
  void apply_phase_one_transformations(int &ch);
  void apply_phase_two_transformations(int &ch);
  
  //Scans the next token or sequence of tokens.
  void scan_next_token();

  /*
   * Returns the next character in the stream without advancing the current position.
   */
  int peek_char()
  {
    return nth_char(1);
  }
  
  /**
   * Saves the current position in the input buffer to allow it to
   * be rewound at a later point.
   */
  void save_current_position()
  {
    mSavedCurrPosition = mCurrPosition;
    mTransformedChars = mTransformedChars;
  }

  /**
   * Restores the current position back to a previously saved state.
   */
  void restore_saved_position()
  {
    mCurrPosition = mSavedCurrPosition;
    mTransformedChars = mSavedTransformedChars;
  }

  /**
   * Discards any previously saved position state.
   */
  void discard_saved_position()
  {
    mSavedCurrPosition = mBuffer.end();
    mSavedTransformedChars.clear();
  }

  /*
   * Indicates whether the end of the input has been reached.
   */
  bool end_of_buffer()
  {
    return mCurrPosition == mBufferEnd
           && mTransformedChars.size() == 0;
  }

public:

  int curr_tok_count() { return mBufferedTokens.size(); }

  /**
   * Constructor. Allows the input string and output interface implementation
   * to be passed in.
   */
  preprocessor_lexer(const string &input)
  {
    mBuffer = input;
    mBufferEnd = mBuffer.cend();
    mCurrPosition = mBuffer.cbegin();
    mSuppressTransformations = 0;
    mLastChar = -1;
    mEndOfFileTokensProcessed = false;
  }

  preprocessor_token next_token();
  bool finished_tokenising();
};

#endif //PREPROCESSOR_LEXER_H

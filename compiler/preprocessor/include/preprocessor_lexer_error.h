#ifndef PREPROCESSOR_LEXER_ERROR_H 
#define PREPROCESSOR_LEXER_ERROR_H

//Thrown when an error is encountered when lexing the input.
class preprocessor_lexer_error : public exception
{
private:
  string mMessage;

public:
  preprocessor_lexer_error(const string &message) : mMessage(message) {}
  ~preprocessor_lexer_error() throw() {}
  const char* what() const throw() { return mMessage.c_str(); }
};

#endif


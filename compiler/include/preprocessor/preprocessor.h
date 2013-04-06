#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "preprocessor_lexer.h"

class preprocessor
{
private:

  preprocessor_lexer mLexer;

public:
  preprocessor_token next_token() { return mLexer.next_token(); }
  bool has_more_tokens() { return !mLexer.finished_tokenising(); }
};


#endif //PREPROCESSOR_H

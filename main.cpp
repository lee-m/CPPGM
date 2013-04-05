#include <iostream>
#include <sstream>
#include <deque>
#include <cstdlib>
using namespace std;

#include "preprocessor_lexer.h"

void write_token(const string& type, const string& data)
{
  cout << type << " " << data.size() << " ";
  cout.write(data.data(), data.size());
  cout << endl;
}

int main()
{
  try
  {
    ostringstream oss;
    oss << cin.rdbuf();

    preprocessor_lexer tokeniser(oss.str());

    while(!tokeniser.finished_tokenising())
    {
      preprocessor_token tok = tokeniser.next_token();

      switch(tok.type)
      {
        case PPTOK_WHITESPACE:
          cout << "whitespace-sequence 0 " << endl;
          break;
  
        case PPTOK_NEW_LINE:
          cout << "new-line 0 " << endl;
          break;

        case PPTOK_HEADER_NAME:
          write_token("header-name", tok.data);
          break;

        case PPTOK_IDENTIFIER:
          write_token("identifier", tok.data);
          break;

        case PPTOK_NUMBER:
          write_token("pp-number", tok.data);
          break;

        case PPTOK_CHAR_LITERAL:
          write_token("character-literal", tok.data);
          break;

        case PPTOK_USER_DEF_CHAR_LITERAL:
          write_token("user-defined-character-literal", tok.data);
          break;

        case PPTOK_STRING_LITERAL:
          write_token("string-literal", tok.data);
          break;

        case PPTOK_USER_DEF_STRING_LITERAL:
          write_token("user-defined-string-literal", tok.data);
          break;

        case PPTOK_PREPROCESSING_OP_OR_PUNC:
          write_token("preprocessing-op-or-punc", tok.data);
          break;

        case PPTOK_NON_WHITESPACE_CHAR:
          write_token("non-whitespace-character", tok.data);
          break;

        case PPTOK_EOF:
          cout << "eof" << endl;
          break;
      }
    }
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


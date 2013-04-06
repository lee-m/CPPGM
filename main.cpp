#include <iostream>
#include <sstream>
#include <cstdlib>
using namespace std;

#include "preprocessor/preprocessor_lexer.h"

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


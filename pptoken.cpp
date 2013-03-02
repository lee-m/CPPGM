#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

using namespace std;

#include "IPPTokenStream.h"
#include "DebugPPTokenStream.h"

// Translation features you need to implement:
// - utf8 decoder
// - utf8 encoder
// - universal-character-name decoder
// - trigraphs
// - line splicing
// - newline at eof
// - comment striping (can be part of whitespace-sequence)

// EndOfFile: synthetic "character" to represent the end of source file
constexpr int EndOfFile = -1;

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
struct PPTokeniser
{
	IPPTokenStream& mOutput;

	//Pointers to the current input buffer, the end of the input buffer and the current position within the buffer
	const char *mBuffer;
	const char *mBufferEnd;
	const char *mCurrPosition;
	char mLastChar;

	//Whether to suppress the standard transformations to apply when lexing each char
	int mSuppressTransformations;

	PPTokeniser(IPPTokenStream& output)
		: mOutput(output)
	{
	}
    
	void process(const string &input)
	{  
		mBuffer = input.c_str();
		mBufferEnd = mBuffer + input.length();
		mCurrPosition = mBuffer;
    mSuppressTransformations = 0;

		skip_whitespace();

		while(!end_of_input())
			{
				int curr_ch = curr_char();
				mLastChar = curr_ch;

				switch(curr_ch)
					{
					case '|':
						{
							//Maybe a bitwise or logical or
							string tok;
							tok.append(1, curr_ch);

							int peeked_ch = peek_char();
							
							if(peeked_ch == '|')
								{
									//Consume the || characters
									skip_chars(2);
									tok.append(1, peeked_ch);
								}
							else
								{
									//Consume the |
									next_char();
								}

							mOutput.emit_preprocessing_op_or_punc(tok);
							break;
						}

						//TODO: disambiguate . as a number or an operator
					case '{': case '}': case '[': case ']': case '#': case '(': case ')': case ';': case ':': case '?':
					case '+': case '-': case '*': case '%': case '^': case '&': case '~': case ',': case '/':
						{
							//Consume the character
							next_char();
							
							//Output what the character represents
							string tok;
							tok.append(1, curr_ch);
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
							next_char();
							mOutput.emit_new_line();
							break;
						}

					case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
					case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
					case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
					case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
					case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
					case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': case '_':
						{
							lex_identifier();
							break;
						}

					case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '.':
						{
							lex_pp_number();
							break;
						}

					case '\\':
						{
							//Consume the '\'
							next_char();

							//If the next character is a new-line, consume the newline and carry on with the next character
							if(!end_of_input() && curr_char() == '\n')
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
						next_char();
					}
			}

		//If the input is not empty and does not end in a new-line, insert one
		if(mBuffer != mBufferEnd 
			 && mLastChar != '\n')
			mOutput.emit_new_line();

		mOutput.emit_eof();
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

	/*
   * pp-number:
	 *   digit
	 *	 . digit
	 *	 pp-number digit
	 *	 pp-number identifier-nondigit
	 *	 pp-number e sign
	 *	 pp-number E sign
	 *	 pp-number .
	 */
	void lex_pp_number()
	{
		string num;

		if(*mCurrPosition == '.')
			{
				num += *mCurrPosition;
				next_char();
				lex_digit_sequence(num);
			}
		else if(isdigit(*mCurrPosition))
			{
				//Handle the digit grammar production
				lex_digit_sequence(num);
				mOutput.emit_pp_number(num);
			}
	}

	void lex_digit_sequence(string &result)
	{
		while(isdigit(*mCurrPosition))
			{
				result += *mCurrPosition;
				next_char();
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
		switch(ch)
			{
			case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
			case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
			case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
			case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
			case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': case '_':
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
				return true;
						
			default:
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
			++mCurrPosition;
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
    //TODO: encode char to UTF8

		if(mCurrPosition == mBufferEnd)
			throw PPTokeniserException("Unexpected end of file found.");
			
		return apply_transformations(*(mCurrPosition + 1));
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
	int nth_char(int pos)
	{
		if(mCurrPosition + pos > mBufferEnd)
			throw PPTokeniserException("Attempt to access past end of input");

		return *(mCurrPosition + pos);
	}

	/** 
   * Advances the current buffer position by the specified number of characters.
   */
	void skip_chars(int count)
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

		//Check for, and fold, trigraphs
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
							}						
					}
			}
		//Check for comemnts
		else if(ch == '/' && peek_char() == '/')
			{
				skip_cpp_comment();
				ch = ' ';
			}
		else if(ch == '/' && peek_char() == '*')
			{
				skip_c_comment();
				ch = ' ';
			}

    //TODO: line splicing, UCNs, encode char to UTF8
		--mSuppressTransformations;
		return ch;
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
		PPTokeniser tokenizer(output);

		tokenizer.process(input);
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


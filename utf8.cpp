#include <vector>
#include <string>
using namespace std;

#include "utf8.h"
#include "preprocessor_lexer_error.h"

/**
 * Encodes the specified character number into UTF-8
 */
int encode_to_utf8(unsigned int char_number)
{
  unsigned char b1 = 0;
  unsigned char b2 = 0;
  unsigned char b3 = 0;
  unsigned char b4 = 0;

  if(char_number < 0x80)
    return char_number & 0x7F;
  else if(char_number < 0x0800)
  {
    b1 = (char_number >> 6 & 0x1F) | 0xC0;
    b2 = (char_number & 0x3F) | 0x80;
  }
  else if(char_number < 0x010000)
  {
    b1 = (char_number >> 12 & 0x0F) | 0xE0;
    b2 = (char_number >> 6 & 0x3F) | 0x80;
    b3 = (char_number & 0x3F) | 0x80;
  }
  else if(char_number < 0x110000)
  {
    b1 = (char_number >> 18 & 0x07) | 0xF0;
    b2 = (char_number >> 12 & 0x3F) | 0x80;
    b3 = (char_number >> 6  & 0x3F) | 0x80;
    b4 = (char_number & 0x3F) | 0x80;
  }

  return (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
}

/**
 * Decodes a UTF8 encoded code point.
 */
int decode_from_utf8(vector<unsigned char> code_units)
{
  unsigned char b1 = 0;
  unsigned char b2 = 0;
  unsigned char b3 = 0;
  unsigned char b4 = 0;

  switch(code_units.size())
  {
    case 2:
    {
      //A two byte UTF8 sequence is represented as 110xxxxx 10yyyyyy
      //so mask out the bits represented by 110 and 10 to leave just
      //the x/y bits set
      b1 = code_units[0] & 0x3f;
      b2 = code_units[1] & 0x7f;

      //Combine the bits from b1 and b2 to form a 16-bit number with
      //the bytes 00000xxx xxyyyyyy where the lower two x bits become the
      //upper two bits of y.The value of this number is the unicode
      //character name
      unsigned char x = b1 >> 2;
      unsigned char y = ((b1 & 0x3) << 6) | b2;

      return x << 8 | y;
    }

    case 3:
    {
      //A three byte UTF8 character is represented as 
      //1110xxxx 10yyyyyy 10zzzzzz so mask out the upper bits of
      //each code units to leave the bits we want
      b1 = code_units[0] & 0xF;
      b2 = code_units[1] & 0x3F;
      b3 = code_units[2] & 0x3F;

      //Combine thhe bits from each byte to form a 16-bit number of 
      //form xxxxyyyy yyzzzzzz whose value is the Unicode character name
      unsigned char x = (b1 << 4) | ((b2 & 0x3C) >> 2);
      unsigned char y = (b2 & 0x3) << 6 | b3;
      return (x << 8) | y;
    }

    case 4:
    {
      //A four byte UTF8 character is represented as
      //11110www 10xxxxxx 10yyyyyy 10zzzzzz so mask out the upper bits
      //of each code unit to leave the bits we're interested in.
      b1 = code_units[0] & 0x3;
      b2 = code_units[1] & 0x3F;
      b3 = code_units[2] & 0x3F;
      b4 = code_units[3] & 0x3F;

      //Combine the bits from each byte to form a 24-bit number with the
      //bytes 000wwwxx xxxxyyyy yyzzzzzz where the bits from x and y are
      //shifted left to form each byte value
      unsigned char x = b1 << 2 | ((b2 & 0x30) >> 4);
      unsigned char y = (b2 & 0xF) << 4 | ((b3 & 0x3C) >> 2);
      unsigned char z = (b3 & 0x3) << 6 | b4;
      return (x << 16) | (y << 8) | z;
    }
    default:
      return 0;

  }

  return (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
}

/**
 * Converts a UTF-8 encoded code unit into its equivalent code points.
 */
void utf8_code_point_to_code_units(int code_point, vector<unsigned char> &code_units)
{
  unsigned char b1 = ((unsigned int)code_point) >> 24;
  unsigned char b2 = (((unsigned int)code_point) >> 16) & 0xFF;
  unsigned char b3 = (((unsigned int)code_point) >> 8) & 0xFFFF;
  unsigned char b4 = ((unsigned int)code_point) & 0xFFFFFF;

  //Always have at least two code units
  code_units.push_back(b1);
  code_units.push_back(b2);

  if(b3)
    code_units.push_back(b3);

  if(b4)
    code_units.push_back(b4);
}

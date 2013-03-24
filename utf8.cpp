#include <iostream>
#include <vector>
using namespace std;

#include "utf8.h"

/**
 * Encodes the specified code unit into UTF-8
 */
int encode_to_utf8(unsigned int code_unit)
{
  unsigned char b1 = 0;
  unsigned char b2 = 0;
  unsigned char b3 = 0;
  unsigned char b4 = 0;

  if(code_unit < 0x80)
    return code_unit & 0x7F;
  else if(code_unit < 0x0800)
    {
      b1 = (code_unit >> 6 & 0x1F) | 0xC0;
      b2 = (code_unit & 0x3F) | 0x80;
    }
  else if(code_unit < 0x010000)
    {
      b1 = (code_unit >> 12 & 0x0F) | 0xE0;
      b2 = (code_unit >> 6 & 0x3F) | 0x80;
      b3 = (code_unit & 0x3F) | 0x80;
    }
  else if(code_unit < 0x110000)
    {
      b1 = (code_unit >> 18 & 0x07) | 0xF0;
      b2 = (code_unit >> 12 & 0x3F) | 0x80;
      b3 = (code_unit >> 6  & 0x3F) | 0x80;
      b4 = (code_unit & 0x3F) | 0x80;
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

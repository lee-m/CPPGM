#ifndef UTF8_H
#define UTF8_H

//utf8.c
int encode_to_utf8(unsigned int code_unit);
void utf8_code_point_to_code_units(int code_point, vector<unsigned char> &code_units);

#endif

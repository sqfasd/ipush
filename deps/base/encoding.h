
#ifndef BASE_ENCODING_H_
#define BASE_ENCODING_H_

bool IsGBKChar(uint8 c1, uint8 c2) {
  if ((c1 >= 0x81 && c1 <= 0xFE) && (c2 >= 0x40 && c2 <= 0xFE)) 
    return true;
  else 
    return false;
}

bool IsUTF8Char(uint8 c1, uint8 c2, uint8 c3) {
  if ((c1 >> 4 == 14) && (c2 >> 6 == 2) && (c3 >> 6 == 2)) 
    return true;
  else 
    return false;
}

#endif // BASE_ENCODING_H_

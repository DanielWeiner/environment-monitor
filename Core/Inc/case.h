#ifndef INC_CASE_H_
#define INC_CASE_H_

static inline char to_lower(char c) {
  if (c >= 'A' && c <= 'Z') {
    return c + ('a' - 'A');
  }
  return c;
}

#endif /* INC_CASE_H_ */
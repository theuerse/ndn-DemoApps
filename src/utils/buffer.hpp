#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

namespace itec
{
class Buffer
{
public:
  Buffer();

  Buffer(char* data, int size);

  Buffer(const Buffer& other);

  ~Buffer();

  char* getData(){return data;}
  int getSize() {return size;}

  void append(Buffer other);
  void append(char* data, int size);

private:

  char* data;
  int size;

};
}
#endif // BUFFER_HPP

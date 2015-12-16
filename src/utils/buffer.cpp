#include "buffer.hpp"

itec::Buffer::Buffer()
{
  data = NULL;
  size = 0;
}

itec::Buffer::~Buffer()
{
  if(data != NULL)
    free(data);
}

itec::Buffer::Buffer(const Buffer& other)
{
  Buffer(other.data, other.size);
}

itec::Buffer::Buffer(char* data, int size)
{
  this->data = (char*)calloc(size, sizeof(char));
  memcpy(this->data,data,size);
  this->size = size;
}

void itec::Buffer::append(Buffer other)
{
  append(other.getData (), other.size);
}

void itec::Buffer::append(char* data, int size)
{
  this->data = (char*)realloc((void*)this->data, (this->size+size) * sizeof(char));
  memcpy(this->data+this->size,data,size);
  this->size +=size;
}


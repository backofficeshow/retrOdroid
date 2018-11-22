#include <cstring>
#include <sstream>

#include "Tap.hxx"

Tap::Tap(const uInt8* image, uInt32 size) : Tape()
{
  myImage = image;
  mySize = size;
}

Tap::~Tap(){
  delete[] myImage;
}

void Tap::reset()
{

}

const uInt8* Tap::getImage(int& size) const
{
  size = mySize;
  return myImage;
}

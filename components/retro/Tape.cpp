#include <cassert>
#include <cstring>
#include <sstream>

#include "bspf.hxx"
#include "Tap.hxx"

Tape* Tape::create(const uInt8* image, uInt32 size) {
  Tape* tape = 0;

  tape = new Tap(image, size);

  return tape;
};

Tape::Tape() {
}

Tape::~Tape()
{

}

Tape::Tape(const Tape& cart)
{
  assert(false);
}

Tape& Tape::operator = (const Tape&)
{
  assert(false);
  return *this;
}

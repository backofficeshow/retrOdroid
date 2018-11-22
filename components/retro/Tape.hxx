#ifndef TAPE_HXX
#define TAPE_HXX

#include <fstream>
#include <sstream>

class Tape;

#include "bspf.hxx"
#include "Array.hxx"

struct FrequencyArea {
  float frequency; float seconds;
};

typedef Common::Array<FrequencyArea> FrequencyAreaList;

class Tape {
  public:
    static Tape* create(const uInt8* image, uInt32 size);
    Tape();
    virtual ~Tape();
    const FrequencyAreaList& frequencyAreas() { return frequencyAreaList; }
  public:
    virtual void reset() = 0;
    virtual const uInt8* getImage(int& size) const = 0;
  protected:

  private:
    FrequencyAreaList frequencyAreaList;
    Tape(const Tape&);
    Tape& operator = (const Tape&);
};

#endif

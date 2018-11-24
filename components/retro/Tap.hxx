#ifndef TAP_HXX
#define TAP_HXX

#include "bspf.hxx"
#include "Tape.hxx"

class Tap : public Tape {
  public:
    Tap(const uInt8* image, uInt32 size);
    virtual ~Tap();
  public:
    void reset();
    void play(); 
    const uInt8* getImage(int& size) const;
  protected:
  private:
    const uInt8* myImage;
    uInt32 mySize;
};

#endif

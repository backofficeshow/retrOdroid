#ifndef TAPE_HXX
#define TAPE_HXX

#include <fstream>
#include <sstream>


extern "C"
{
  #include "../odroid/odroid_audio.h"
}
class Tape;

#include "bspf.hxx"

#define AUDIO_SAMPLE_RATE (44100)

#define I2S_SAMPLE_RATE   (44100)
#define SAMPLERATE I2S_SAMPLE_RATE // Sample rate of our waveforms in Hz
#define AMPLITUDE             1024

static ODROID_AUDIO_SINK AudioSink = ODROID_AUDIO_SINK_DAC;

class Tape {
  public:
    static Tape* create(const uInt8* image, uInt32 size);
    Tape();
    virtual ~Tape();
  public:
    virtual void reset() = 0;
    virtual void play() = 0;
    virtual const uInt8* getImage(int& size) const = 0;
    void initAudio();
    void changeDac();
    void playFreq(int amplitude, float frequency, float seconds);
  protected:

  private:
    Tape(const Tape&);
    Tape& operator = (const Tape&);
};

#endif

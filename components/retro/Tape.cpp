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

void Tape::initAudio(){
  //odroid_audio_init(AudioSink, AUDIO_SAMPLE_RATE);
  //odroid_audio_terminate();
  odroid_audio_init(AudioSink, AUDIO_SAMPLE_RATE);
  odroid_audio_volume_set(ODROID_VOLUME_LEVEL4);
  short outbuf[2] = {0x00,0x00};
  odroid_audio_submit(outbuf, 1);
  printf("AUDIO INIT [%d]\n", AudioSink);
}

void Tape::changeDac(){
  if (AudioSink == ODROID_AUDIO_SINK_DAC){
    AudioSink = ODROID_AUDIO_SINK_SPEAKER;
  } else if (AudioSink == ODROID_AUDIO_SINK_SPEAKER){
    AudioSink = ODROID_AUDIO_SINK_DAC;
  }
  initAudio();
}

void Tape::playFreq(int amplitude, float frequency, float seconds) {
    short outbuf[2] = {0x00,0x00};
    uint32_t iterations = (seconds * AUDIO_SAMPLE_RATE);
    int halfWavelength = iterations / 2;
    int halfWavelength2 = halfWavelength;

    short sample = 32767 / 1.5;
    int sw = 0;
    for (uint32_t i=0; i<iterations; ++i) {
      if (i % halfWavelength2 == 0){
        sw = !sw;
      }
      outbuf[0] = sw ? sample : -sample;
      outbuf[1] = outbuf[0];
      odroid_audio_submit(outbuf, 1);
    }
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

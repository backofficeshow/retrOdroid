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
  odroid_audio_init(AudioSink, AUDIO_SAMPLE_RATE);
  odroid_audio_terminate();
  odroid_audio_init(AudioSink, AUDIO_SAMPLE_RATE);
  odroid_audio_volume_set(ODROID_VOLUME_LEVEL0);
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
    odroid_audio_volume_set(ODROID_VOLUME_LEVEL4);
    short outbuf[2];
    int halfWavelength = (SAMPLERATE / frequency);
    uint32_t iterations = seconds*SAMPLERATE;
    int count = 0;
    short sample = amplitude;
    for (uint32_t i=0; i<iterations; ++i) {
      if (count % halfWavelength == 0) {
        // invert the sample every half wavelength count multiple to generate square wave
        sample = -1 * sample;
      }
      outbuf[0] = sample;
      outbuf[1] = sample;
      odroid_audio_submit(outbuf, 1);
      count++;
    }
    odroid_audio_volume_set(ODROID_VOLUME_LEVEL0);
    odroid_audio_submit(outbuf, 1);
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

#include <cstring>
#include <sstream>

#include "Tap.hxx"

#define PILOTLENGTH           619.0
#define SYNCFIRST             191.0
#define SYNCSECOND            210.0
#define ZEROPULSE             244.0
#define ONEPULSE              489.0
#define PILOTNUMBERL          8063.0
#define PILOTNUMBERH          3223.0
#define PAUSELENGTH           1000.0

#define MS                    1000000.0

//3540000 / t-state / 2
#define NO_FREQ               0.0
#define PILOT_FREQ            816.42
#define SYNCFIRST_FREQ        2653.67
#define SYNCSECOND_FREQ       2408.16
#define ZERO_PULSE_FREQ       2070.18
#define ONE_PULSE_FREQ        1035.09

const char TAPcheck[7] = {'T','A','P','t','a','p','.'};

Tap::Tap(const uInt8* image, uInt32 size) : Tape()
{
  myImage = image;
  mySize = size;
}

void Tap::play()
{
  initAudio();

  printf("READ SIZE: [%d]\n", mySize);

  //decode TAP file
  for (int i = 0; i < mySize; ++i) {
    printf("READ AT: [%d]\n", i);
    uInt16 length =  ( myImage[i+1] << 8 ) | myImage[i+0];
    i=i+2;

    uint8_t type = myImage[i];

    printf("READ BLOCK TYPE [%d] [%d] SIZE: [%d]\n", i, type, length);

    //build pilot
    printf("  PILOT [%f] \n", ((type == 0) ? PILOTNUMBERL : PILOTNUMBERH)) ;
    for (int p = ((type == 0) ? PILOTNUMBERL : PILOTNUMBERH); p >= 0 ; p--) {
      playFreq(AMPLITUDE, ( p % 2 == 0) ? PILOT_FREQ : NO_FREQ, PILOTLENGTH / MS);
    }

    //build sync1
    printf("  SYNC 1\n");
    playFreq(AMPLITUDE, SYNCFIRST_FREQ, SYNCFIRST / MS);
    //playFreq(AMPLITUDE, NO_FREQ, SYNCFIRST / MS);

    //build sync2
    printf("  SYNC 2\n");
    playFreq(AMPLITUDE, SYNCSECOND_FREQ, SYNCSECOND / MS);
    playFreq(AMPLITUDE, NO_FREQ, SYNCSECOND / MS);
    printf("  DATA\n");
    for (int d = 0; d < length; ++d) {
      //printf("    READ DATA: [%d] [%d]\n",d , i+d);
      uint8_t byte = myImage[i+d];
      for (short ii=8; ii >=0; --ii){
        playFreq(AMPLITUDE, ((byte >> ii) & 0x01) ? ONE_PULSE_FREQ : ZERO_PULSE_FREQ, (((byte >> ii) & 0x01) ?  ONEPULSE : ZEROPULSE) / MS);
      }
    }
    printf(" FIN READ BLOCK: [%d] [%d]\n", i, length);
    i=i+length+1;
    playFreq(AMPLITUDE, NO_FREQ, 1);
  }
  printf("PLAY TAP DONE\n");

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

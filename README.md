# retrOdroid
Odroid-go Retro Tape Interface (Uses the Dr A's DAC hat from www.backofficeshow.com/shop)

Allows you to use the roms you have installed for all you favourite tape based titles and play the tape data out of the Dr A's odroid-go headphone DAC interface. We will be adding system by system and tape-artwork features as well. Cool eh?

Assumes you have installed the toolchain:

https://docs.espressif.com/projects/esp-idf/en/stable/get-started/

then at the command line you will need to run:

make flash

or

make flash monitor

This will enable the serial output from the ODroid



For Reference to Make a Firmware File

git clone https://github.com/othercrashoverride/odroid-go-firmware.git -b factory
cd odroid-go-firmware/tools/mkfw
make

sudo apt install ffmpeg
ffmpeg -i 86x48px-Logo.png -f rawvideo -pix_fmt rgb565 tile.raw


./mkfw test tile.raw 0 16 1048576 app Hello_World.bin

mv firmware.fw Hello_World.fw
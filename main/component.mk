#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the
# src/ directory, compile them and link them into lib(subdirectory_name).a
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#
CFLAGS += -DSOUND_SUPPORT=1
COMPONENT_ADD_INCLUDEDIRS += ../components/common ../components/retro ../components/stella/gui

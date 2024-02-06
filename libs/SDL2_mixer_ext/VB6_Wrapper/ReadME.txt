=================================================================
     Wrapper of SDL Mixer Ext API for Visual Basic 6
=====        21 January 2017 by Vitaly Novichkov            =====
=================================================================
= You are allowed to use and modify this wrapper without limits =
= Everything is presented as-is, and I don't take resonse for   = 
= any damages caused by usage of this wrapper.                  =
=================================================================

1) SDL2MixerVB.dll is required
2) Working pre-built example in the "build" folder
3) It's going as non-COM!
4) To have your project working, you should include modSDL2_mixer_ext_vb6.bas
5) Full documentation of SDL Mixer X is here: https://wohlsoft.github.io/SDL-Mixer-X/SDL_mixer_ext.html

Functions are has similar name and arguments. BUT, some exceptions:
- Pointers to Mix_Music* and Mix_Chunk* are presented as Long variable
- Some functions are not binded into this wrapper because there are useless or incompatible
- There is no binded SDL functions except some necessary functions, required for SDL Mixer X usage

Note about example project:
- to make it working, you shuld put all files from example_media and dlls folders
  into working directory of built application.


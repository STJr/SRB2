#include <emu_de_midi.h>

int main()
{
    struct EDMIDIPlayer *t = edmidi_init(48000);
    edmidi_close(t);
    return 0;
}

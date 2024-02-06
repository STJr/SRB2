#include <opnmidi.h>

int main()
{
    struct OPN2_MIDIPlayer *t = opn2_init(48000);
    opn2_close(t);
    return 0;
}

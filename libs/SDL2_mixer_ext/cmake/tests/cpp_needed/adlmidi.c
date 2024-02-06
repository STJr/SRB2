#include <adlmidi.h>

int main()
{
    struct ADL_MIDIPlayer *t = adl_init(48000);
    adl_close(t);
    return 0;
}

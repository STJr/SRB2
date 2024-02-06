#ifdef MODPLUG_HEADER
#include MODPLUG_HEADER
#else
#include <libmodplug/modplug.h>
#endif

int main()
{
    ModPlug_Load(0, 0);
    return 0;
}

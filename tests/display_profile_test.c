#include "app_config.h"
#include "input_map.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

InputMap g_input_map;
const char *input_button_name(GenesisButton button)
{
    (void)button;
    return "unused";
}

static void launcher_mode(const char *value)
{
#ifdef _WIN32
    assert(_putenv_s("RECOMPLAUNCHER_MODE", value ? value : "") == 0);
#else
    assert((value ? setenv("RECOMPLAUNCHER_MODE", value, 1)
                  : unsetenv("RECOMPLAUNCHER_MODE")) == 0);
#endif
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    const char *settings = argv[1];

    launcher_mode(NULL);
    app_config_defaults();
    assert(g_app_config.fullscreen == 1);
    launcher_mode("desktop");
    app_config_defaults();
    assert(g_app_config.fullscreen == 0 && g_app_config.fullscreen_auto);
    assert(app_config_save(settings));

    launcher_mode("bigpicture");
    app_config_defaults();
    assert(app_config_load(settings));
    assert(g_app_config.fullscreen == 1 && g_app_config.fullscreen_auto);

    g_app_config.fullscreen = 2;
    g_app_config.fullscreen_auto = 0;
    assert(app_config_save(settings));
    launcher_mode("desktop");
    app_config_defaults();
    assert(app_config_load(settings));
    assert(g_app_config.fullscreen == 2 && !g_app_config.fullscreen_auto);

    assert(remove(settings) == 0);
    return 0;
}

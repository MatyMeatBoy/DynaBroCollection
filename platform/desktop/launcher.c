#include <SDL.h>
#include <errno.h>
#include <locale.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include "launcher_discovery.h"
#include "launcher_options.h"
#include "launcher_profile.h"

extern char **environ;

static int launcher_dir(char *path, size_t capacity)
{
#ifdef __APPLE__
    uint32_t size = (uint32_t)capacity;
    if (_NSGetExecutablePath(path, &size)) return 0;
    char *canonical = realpath(path, NULL);
    if (!canonical) return 0;
    int length = snprintf(path, capacity, "%s", canonical);
    free(canonical);
    if (length < 0 || (size_t)length >= capacity) return 0;
#else
    ssize_t length = readlink("/proc/self/exe", path, capacity - 1);
    if (length <= 0 || (size_t)length >= capacity - 1) return 0;
    path[length] = '\0';
#endif
    char *slash = strrchr(path, '/');
    if (!slash) return 0;
    slash[1] = '\0';
    return 1;
}

static int select_game(void)
{
    const SDL_MessageBoxButtonData choices[] = {
        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Close"},
        {0, 2, "Dyna Brothers 2 Special"},
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Dyna Brothers"}
    };
    const SDL_MessageBoxData box = {
        SDL_MESSAGEBOX_INFORMATION, NULL, "DynaBroCollection",
        "Choose a game. You can select your ROM in its setup screen.",
        sizeof choices / sizeof *choices, choices, NULL
    };
    SDL_SetMainReady();
    int game = -1;
    if (SDL_ShowMessageBox(&box, &game) < 0) {
        fprintf(stderr, "Could not open the collection: %s\n", SDL_GetError());
        return -2;
    }
    return game;
}

static int start_game(const char *directory, const DynaLauncherOptions *options, const char *rom)
{
    char exe[4096], mode[32];
    int n = snprintf(exe, sizeof exe, "%sDynaBrothers%dAOT", directory, options->game);
    if (n < 0 || (size_t)n >= sizeof exe) return 1;
    char *child[9];
    size_t count = 0;
    child[count++] = exe;
    if (options->no_launcher) child[count++] = "--no-launcher";
    if (options->setup) child[count++] = "--launcher";
    if (options->fullscreen) child[count++] = "--fullscreen";
    if (options->widescreen) {
        if (!ld_wide_to_utf8(options->widescreen, mode, sizeof mode)) return 2;
        child[count++] = "--widescreen";
        child[count++] = mode;
    }
    if (rom) child[count++] = (char *)rom;
    child[count] = NULL;

    pid_t process;
    int spawn_error = posix_spawn(&process, exe, NULL, NULL, child, environ);
    if (spawn_error) {
        fprintf(stderr, "Cannot start %s: %s\n", exe, strerror(spawn_error));
        return 1;
    }
    int status;
    while (waitpid(process, &status, 0) < 0) {
        if (errno != EINTR) {
            perror("waitpid");
            return 1;
        }
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}

int main(int argc, char **argv)
{
    if (!setlocale(LC_CTYPE, "") || MB_CUR_MAX == 1) {
        if (!setlocale(LC_CTYPE, "C.UTF-8")) (void)setlocale(LC_CTYPE, "UTF-8");
    }
    const wchar_t **wide = calloc((size_t)argc, sizeof(*wide));
    wchar_t (*storage)[LD_PATH_CAP] = calloc((size_t)argc, sizeof(*storage));
    if (!wide || !storage) {
        free(storage);
        free(wide);
        return 1;
    }
    int result = 2;
    for (int i = 0; i < argc; i++) {
        if (!ld_utf8_to_wide(argv[i], storage[i], LD_PATH_CAP)) {
            fputs("Invalid UTF-8 or an oversized launch argument.\n", stderr);
            goto done;
        }
        wide[i] = storage[i];
    }
    DynaLauncherOptions options;
    wchar_t error[256];
    if (!dyna_launcher_options_parse(argc, wide, &options, error, sizeof error / sizeof *error)) {
        fprintf(stderr, "%ls\n", error);
        goto done;
    }
    if (options.help || options.integration_info) {
        if (options.help) printf("%ls", dyna_launcher_options_help());
        if (options.integration_info) puts(dyna_launcher_integration_info());
        result = 0;
        goto done;
    }

    char directory[4096], profile[4096], rom[LD_PATH_CAP * 4];
    if (!launcher_dir(directory, sizeof directory)) {
        fputs("Cannot locate the collection installation.\n", stderr);
        result = 1;
        goto done;
    }
    if (options.rom) {
        ld_item item;
        result = ld_probe_rom(options.rom, options.game, NULL, &item);
        if (result != LD_ROM_OK) {
            fprintf(stderr, "%s\n", ld_rom_error(result, options.game));
            goto done;
        }
        if (!ld_wide_to_utf8(item.path, rom, sizeof rom)) {
            result = LD_ROM_UNREADABLE;
            goto done;
        }
        options.game = item.game;
    }
    if (!options.game) {
        options.game = select_game();
        if (options.game <= 0) {
            result = options.game == -2 ? 1 : 0;
            goto done;
        }
    }
#ifdef __APPLE__
    const char *portable = NULL;
#else
    const char *portable = directory;
#endif
    if (!dyna_profile_resolve(portable, profile, sizeof profile) || setenv("GENESIS_USER_DIR", profile, 1)) {
        fputs("Cannot create a writable DynaBroCollection profile.\n", stderr);
        result = 1;
        goto done;
    }
    result = start_game(directory, &options, options.rom ? rom : NULL);

done:
    free(storage);
    free(wide);
    return result;
}

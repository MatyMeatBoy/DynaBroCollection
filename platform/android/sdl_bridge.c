#include <SDL.h>
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "game_spec.h"
#include "state_crc.h"

extern const GameSpec g_game_spec;

#if DYNA_ANDROID_GAME_ID == 1
#define DYNA_VALIDATE Java_org_dynabro_collection_LibraryActivity_validateFirstRom
#else
#define DYNA_VALIDATE Java_org_dynabro_collection_LibraryActivity_validateSecondRom
#endif

JNIEXPORT jboolean JNICALL DYNA_VALIDATE(JNIEnv *env, jclass type, jstring path)
{
    (void)type;
    const char *filename = (*env)->GetStringUTFChars(env, path, NULL);
    if (!filename) return JNI_FALSE;
    FILE *rom = fopen(filename, "rb");
    (*env)->ReleaseStringUTFChars(env, path, filename);
    if (!rom) return JNI_FALSE;

    uint8_t buffer[16384];
    uint32_t crc = UINT32_MAX;
    size_t total = 0, count;
    while ((count = fread(buffer, 1, sizeof(buffer), rom)) != 0) {
        total += count;
        if (total > g_game_spec.expected_rom_size) break;
        crc = genesis_state_crc32_update(crc, buffer, count);
    }
    int valid_size = !ferror(rom) && total == g_game_spec.expected_rom_size;
    fclose(rom);
    if (!valid_size) return JNI_FALSE;

    crc = ~crc;
    if (crc == g_game_spec.expected_rom_crc32) return JNI_TRUE;
    for (size_t i = 0; i < g_game_spec.num_rom_aliases; ++i) {
        if (crc == g_game_spec.rom_aliases[i].crc32) return JNI_TRUE;
    }
    return JNI_FALSE;
}

#ifdef SDL_MAIN_HANDLED
extern int main(int argc, char **argv);

/* RecompLauncher builds the runner's regular main. SDLActivity needs SDL_main. */
DECLSPEC int SDLCALL SDL_main(int argc, char **argv)
{
    SDL_SetMainReady();
    return main(argc, argv);
}
#endif

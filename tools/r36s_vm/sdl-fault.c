/* SDL2 ABI hooks for the isolated ARM64 test VM. */
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Rect SDL_Rect;
struct timespec { long tv_sec, tv_nsec; };
extern void *dlsym(void *handle, const char *name);
extern char *getenv(const char *name);
extern int clock_gettime(int clock, struct timespec *time);
extern int nanosleep(const struct timespec *request, struct timespec *remaining);
extern long write(int fd, const void *data, unsigned long size);
extern int dprintf(int fd, const char *format, ...);
extern void qsort(void *base, unsigned long count, unsigned long size,
                  int (*compare)(const void *, const void *));
extern int SDL_SetError(const char *format, ...);

static unsigned long last_present_us;
static unsigned long present_intervals[8192];
static unsigned present_samples;

static int compare_interval(const void *a, const void *b)
{
    unsigned long first = *(const unsigned long *)a;
    unsigned long second = *(const unsigned long *)b;
    return (first > second) - (first < second);
}

__attribute__((destructor)) static void report_present_intervals(void)
{
    unsigned n = present_samples;
    if (!n)
        return;
    qsort(present_intervals, n, sizeof(*present_intervals), compare_interval);
    dprintf(2, "[SDL-PRESENT] samples=%u p50_us=%lu p95_us=%lu p99_us=%lu max_us=%lu\n",
            n, present_intervals[(n * 50 + 99) / 100 - 1],
            present_intervals[(n * 95 + 99) / 100 - 1],
            present_intervals[(n * 99 + 99) / 100 - 1], present_intervals[n - 1]);
}

static char fault_mode(void)
{
    const char *mode = getenv("R36S_SDL_FAULT");
    return mode ? mode[0] : 0;
}

int SDL_UpdateTexture(SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch)
{
    static int (*update)(SDL_Texture *, const SDL_Rect *, const void *, int);
    static unsigned uploads;
    if (!update)
        update = dlsym((void *)-1, "SDL_UpdateTexture");
    if (fault_mode() == 'u' && ++uploads % 30 == 0) {
        static const char message[] = "[SDL-FAULT] texture upload rejected\n";
        write(2, message, sizeof(message) - 1);
        SDL_SetError("Injected texture upload failure");
        return -1;
    }
    return update ? update(texture, rect, pixels, pitch) : -1;
}

void SDL_RenderPresent(SDL_Renderer *renderer)
{
    static void (*present)(SDL_Renderer *);
    static int logged;
    if (!present)
        present = dlsym((void *)-1, "SDL_RenderPresent");
    if (fault_mode() == 'p') {
        const struct timespec delay = {0, 25000000};
        if (!logged) {
            static const char message[] = "[SDL-FAULT] adding 25 ms per present\n";
            write(2, message, sizeof(message) - 1);
            logged = 1;
        }
        nanosleep(&delay, 0);
    }
    if (present) {
        present(renderer);
        struct timespec time;
        if (!clock_gettime(1, &time)) {
            unsigned long now = (unsigned long)time.tv_sec * 1000000 + time.tv_nsec / 1000;
            if (last_present_us && present_samples < sizeof(present_intervals) / sizeof(*present_intervals))
                present_intervals[present_samples++] = now - last_present_us;
            last_present_us = now;
        }
    }
}

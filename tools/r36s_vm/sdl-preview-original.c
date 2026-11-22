/* Display/input bridge for the SDL2 ARM64 port inside the local QEMU lab. */
typedef struct SDL_Renderer SDL_Renderer;
struct pollfd { int fd; short events, revents; };
extern void *dlsym(void *, const char *);
extern char *getenv(const char *);
extern int open(const char *, int, ...);
extern long read(int, void *, unsigned long);
extern long write(int, const void *, unsigned long);
extern int poll(struct pollfd *, unsigned long, int);
extern void *memcpy(void *, const void *, unsigned long);
extern int SDL_RenderReadPixels(SDL_Renderer *, const void *, unsigned, void *, int);
extern int SDL_GetRendererOutputSize(SDL_Renderer *, int *, int *);
extern int SDL_PushEvent(void *);

static int channel = -1;
static unsigned keys;
static unsigned low_byte;
static int expect_high;
static int closing;
static int input_seen;
static unsigned pixels[640 * 480];
static unsigned char frame[320 * 240 * 3];

static int get_channel(void)
{
    if (channel < 0) {
        const char *path = getenv("R36S_PREVIEW_PORT");
        if (path)
            channel = open(path, 2);
    }
    return channel;
}

static void update_keys(void)
{
    int fd = get_channel();
    if (fd < 0)
        return;
    struct pollfd entry = {fd, 1, 0};
    if (poll(&entry, 1, 0) <= 0 || !(entry.revents & 1))
        return;
    unsigned char bytes[64];
    long count = read(fd, bytes, sizeof(bytes));
    for (long i = 0; i < count; ++i) {
        if (expect_high)
            keys = low_byte | (unsigned)bytes[i] << 8;
        else
            low_byte = bytes[i];
        expect_high = !expect_high;
    }
    if ((keys & 255) && !input_seen) {
        static const char message[] = "R36S_PREVIEW_KEYBOARD_OK\n";
        write(2, message, sizeof(message) - 1);
        input_seen = 1;
    }
    if ((keys & 256) && !closing) {
        union { unsigned type; unsigned long align; unsigned char data[56]; } event = {0};
        event.type = 0x100; /* SDL_QUIT */
        closing = 1;
        SDL_PushEvent(&event);
    }
}

const unsigned char *SDL_GetKeyboardState(int *count)
{
    static const unsigned char *(*original)(int *);
    static unsigned char state[512];
    static const unsigned scan[] = {82, 81, 80, 79, 29, 27, 6, 40};
    int length = 0;
    if (!original)
        original = dlsym((void *)-1, "SDL_GetKeyboardState");
    const unsigned char *real = original(&length);
    if (count)
        *count = length;
    if (length != (int)sizeof(state))
        return real;
    update_keys();
    memcpy(state, real, sizeof(state));
    for (unsigned i = 0; i < sizeof(scan) / sizeof(scan[0]); ++i)
        if (keys & (1u << i))
            state[scan[i]] = 1;
    return state;
}

static void send_frame(const unsigned char *data, unsigned long size)
{
    while (size) {
        long count = write(channel, data, size);
        if (count <= 0)
            return;
        data += count;
        size -= (unsigned long)count;
    }
}

void SDL_RenderPresent(SDL_Renderer *renderer)
{
    static void (*present)(SDL_Renderer *);
    static unsigned picture;
    int width = 0, height = 0;
    if (!present)
        present = dlsym((void *)-1, "SDL_RenderPresent");
    if (++picture % 2 == 0 && get_channel() >= 0 &&
        SDL_GetRendererOutputSize(renderer, &width, &height) == 0 && width == 640 && height == 480 &&
        SDL_RenderReadPixels(renderer, 0, 0x16362004u, pixels, 640 * 4) == 0) {
        for (unsigned y = 0; y < 240; ++y) for (unsigned x = 0; x < 320; ++x) {
            unsigned color = pixels[y * 1280 + x * 2];
            unsigned index = (y * 320 + x) * 3;
            frame[index] = (unsigned char)(color >> 16);
            frame[index + 1] = (unsigned char)(color >> 8);
            frame[index + 2] = (unsigned char)color;
        }
        send_frame(frame, sizeof(frame));
    }
    present(renderer);
}

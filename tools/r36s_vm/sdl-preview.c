/* Display/input bridge for the SDL2 ARM64 port inside the local QEMU lab. */
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Joystick SDL_Joystick;
typedef struct SDL_GameController SDL_GameController;
struct joystick_guid { unsigned char bytes[16]; };
struct rect { int x, y, w, h; };
struct pollfd { int fd; short events, revents; };
struct timespec { long tv_sec, tv_nsec; };
extern void *dlsym(void *, const char *);
extern char *getenv(const char *);
extern int open(const char *, int, ...);
extern long read(int, void *, unsigned long);
extern long write(int, const void *, unsigned long);
extern int poll(struct pollfd *, unsigned long, int);
extern void *memcpy(void *, const void *, unsigned long);
extern int SDL_PushEvent(void *);
extern int SDL_RenderSetVSync(SDL_Renderer *, int);
extern void *memset(void *, int, unsigned long);
extern int clock_gettime(int, struct timespec *);
extern int dprintf(int, const char *, ...);
extern int snprintf(char *, unsigned long, const char *, ...);
extern int SDL_JoystickAttachVirtual(int, int, int, int);
extern SDL_Joystick *SDL_JoystickOpen(int);
extern struct joystick_guid SDL_JoystickGetDeviceGUID(int);
extern void SDL_JoystickGetGUIDString(struct joystick_guid, char *, int);
extern int SDL_GameControllerAddMapping(const char *);
extern SDL_GameController *SDL_GameControllerOpen(int);
extern unsigned char SDL_GameControllerGetButton(SDL_GameController *, int);
extern int SDL_JoystickSetVirtualButton(SDL_Joystick *, int, unsigned char);
extern void SDL_JoystickUpdate(void);

static int channel = -1;
static unsigned keys;
static unsigned low_byte;
static int expect_high;
static int closing;
static int input_seen;
static unsigned frame[320 * 240];
static unsigned long sent;
static int pending;
static unsigned picture;
static unsigned preview_skip_mask = 1;
static unsigned long rate_start;
static SDL_Joystick *virtual_pad;
static SDL_GameController *pad_controller;
static unsigned pad_applied;
static int pad_ready;
static int pad_start_seen;

static void setup_pad(void)
{
    if (pad_ready)
        return;
    pad_ready = 1;
    if (!getenv("R36S_PREVIEW_PAD"))
        return;
    int device = SDL_JoystickAttachVirtual(1, 4, 17, 0);
    if (device < 0)
        return;
    char guid[33], mapping[360];
    SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(device), guid, (int)sizeof(guid));
    snprintf(mapping, sizeof(mapping),
        "%s,R36S Lab Gamepad,a:b0,b:b1,x:b2,y:b3,back:b12,guide:b16,start:b13,"
        "leftshoulder:b4,rightshoulder:b5,dpup:b8,dpdown:b9,dpleft:b10,dpright:b11,"
        "leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:b6,righttrigger:b7,platform:Linux,",
        guid);
    if (SDL_GameControllerAddMapping(mapping) < 0)
        return;
    virtual_pad = SDL_JoystickOpen(device);
    pad_controller = SDL_GameControllerOpen(device);
    if (virtual_pad && pad_controller)
        dprintf(2, "R36S_PREVIEW_VIRTUAL_PAD_READY\n");
}

static void apply_pad(void)
{
    if (!virtual_pad || pad_applied == keys)
        return;
    static const unsigned source_bits[] = {0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15};
    static const unsigned buttons[] = {8, 9, 10, 11, 1, 0, 2, 13, 12, 4, 5, 3, 6, 7, 16};
    for (unsigned i = 0; i < sizeof(buttons) / sizeof(buttons[0]); ++i) {
        unsigned mask = 1u << source_bits[i];
        if ((pad_applied ^ keys) & mask)
            SDL_JoystickSetVirtualButton(virtual_pad, (int)buttons[i], (keys & mask) != 0);
    }
    pad_applied = keys;
    SDL_JoystickUpdate();
    if (keys & 0xfeffu)
        dprintf(2, "[R36S-PAD] mask=%04x\n", keys);
    if (!pad_start_seen && (keys & (1u << 7)) && pad_controller &&
        SDL_GameControllerGetButton(pad_controller, 6)) {
        dprintf(2, "R36S_PREVIEW_SDL_START_OK\n");
        pad_start_seen = 1;
    }
}

static int get_channel(void)
{
    if (channel < 0) {
        const char *path = getenv("R36S_PREVIEW_PORT");
        if (path) {
            channel = open(path, 2 | 2048); /* O_RDWR | O_NONBLOCK on Linux AArch64. */
            preview_skip_mask = getenv("R36S_PREVIEW_FULL_RATE") ? 0 : 1;
        }
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
        dprintf(2, "%s\n", virtual_pad ? "R36S_PREVIEW_PAD_INPUT_OK" : "R36S_PREVIEW_KEYBOARD_OK");
        input_seen = 1;
    }
    apply_pad();
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
    if (virtual_pad)
        return real;
    memcpy(state, real, sizeof(state));
    for (unsigned i = 0; i < sizeof(scan) / sizeof(scan[0]); ++i)
        if (keys & (1u << i))
            state[scan[i]] = 1;
    return state;
}

static void flush_frame(void)
{
    while (pending && sent < sizeof(frame)) {
        long count = write(channel, (const unsigned char *)frame + sent, sizeof(frame) - sent);
        if (count <= 0)
            return;
        sent += (unsigned long)count;
    }
    pending = 0;
    sent = 0;
}

int SDL_UpdateTexture(SDL_Texture *texture, const struct rect *rect, const void *pixels, int pitch)
{
    static int (*update)(SDL_Texture *, const struct rect *, const void *, int);
    if (!update)
        update = dlsym((void *)-1, "SDL_UpdateTexture");
    if ((picture & preview_skip_mask) == 0 && (!pending || sent == 0) && get_channel() >= 0 &&
        rect && pixels && rect->x == 0 && rect->y == 0 &&
        (rect->w == 320 || rect->w == 256) && rect->h > 0 && rect->h <= 240 && pitch >= rect->w * 4) {
        unsigned top = (240 - (unsigned)rect->h) / 2;
        memset(frame, 0, top * 320 * sizeof(*frame));
        memset(frame + (top + (unsigned)rect->h) * 320, 0,
               (240 - top - (unsigned)rect->h) * 320 * sizeof(*frame));
        for (unsigned y = 0; y < (unsigned)rect->h; ++y) {
            const unsigned *source = (const unsigned *)((const unsigned char *)pixels + y * (unsigned)pitch);
            unsigned *target = frame + (y + top) * 320;
            if (rect->w == 320) {
                memcpy(target, source, 320 * sizeof(*target));
            } else {
                for (unsigned x = 0; x < 320; ++x)
                    target[x] = source[(x * 4) / 5];
            }
        }
        pending = 1;
    }
    return update(texture, rect, pixels, pitch);
}

void SDL_RenderPresent(SDL_Renderer *renderer)
{
    static void (*present)(SDL_Renderer *);
    static SDL_Renderer *pacer_renderer;
    if (!present)
        present = dlsym((void *)-1, "SDL_RenderPresent");
    if (pacer_renderer != renderer) {
        if (!getenv("R36S_PREVIEW_DEFER_VSYNC"))
            SDL_RenderSetVSync(renderer, 0);
        pacer_renderer = renderer;
    }
    setup_pad();
    if (picture++ % 180 == 0) {
        struct timespec now;
        if (clock_gettime(1, &now) == 0) {
            unsigned long tick = (unsigned long)now.tv_sec * 1000000 + now.tv_nsec / 1000;
            if (rate_start && tick > rate_start)
                dprintf(2, "[R36S-LIVE] presentations=180 elapsed_us=%lu\n", tick - rate_start);
            rate_start = tick;
        }
    }
    flush_frame();
    present(renderer);
}

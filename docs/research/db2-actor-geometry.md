# DB2 Special actor geometry and immutable decoding

Evidence: Japanese Virtual Console Special image SHA-256 `1d3f08e235fa8e51b9656820fabfb5b703acaf36bc34d5378a3f9613d453bb76`. Addresses below are ROM PCs or 24-bit guest RAM addresses. No DB1 constants are used.

## Records, occupancy, and native limits

`01E0BE..01E0EC` initializes 256 records of 48 bytes at `FF5F58`, writing each ordinal to record `+10`. Allocation groups from `01E118` begin at IDs 1, 8, 108, 148, 248 and have lengths 7,100,40,100,7. Thus normal IDs are 1..254; zero is the empty/sentinel ID, and FF identifies pseudo effects in the native visible list. The update loop `01812A..01821C` examines records 1..255, but FF is not an allocatable body ID.

Record fields verified by consumers:

| Offset | Meaning |
|---|---|
|00|type, zero inactive; graphics/animation/palette maps have 92 entries (types 1..92)|
|01|direction/variant; bit0 ignored for animation selection|
|02,03|map cell X,Y|
|04,05|signed subcell X,Y|
|06|signed flight/displacement Y; excluded from depth|
|07|animation state|
|08|elapsed animation time used when initializing a visual cache entry|
|0A|two-byte cell index|
|0E|native visible-list node offset|
|10|actor ID|
|11|visual flags; bits7/6 apply guest-generated mask overlays|
|12|native visual-cache entry offset|
|28|six marker flags|

Occupancy is `FF8FAA + 2*(40*y+x)`, two one-byte IDs per cell. `01E72E..01E754` writes record10 into the first free byte and then the second; `01E174` clears 3200 bytes. `FFD20A` is a separate blocker layer, not this actor occupancy table.

The native list at `FF9D3A` has a four-byte header and 72 eight-byte nodes. Header +0 is head, +2 tail. Node +0 next, +2 actor ID, +3 pseudo index when ID FF, +4 signed ground depth, +6 previous. Link zero terminates. `01F00E` inserts actors, `01F080` inserts pseudo effects, and `01F258` repairs order. Depth is `32*record03 + s8(record05)`; flight is absent. Head-to-tail order is descending depth. Equality follows insertion/update history, not a universal actor-ID sort.

`0146C0..01485A` admits only the first 40 list entries into 40 visual-cache entries of 32 bytes at `FF1BB2`. Pattern LRU entries are 40*12 bytes at `FF256E`, and `014A00..014DAA` has a 40-pattern budget, including cache hits. A negative visual-cache word +4 suppresses the body (`014E7C`). These limits cannot supply an expanded view; immutable decoding must enumerate all actor records.

## Projection and components

The camera pointer is RAM `FF2A56`; it must point into guest RAM, with coarse X,Y bytes at +0,+1. `014E84..014EDA` computes pre-SAT body coordinates:

```
x = 32*s8(record02-cameraX) + s8(record04+animationDX)
    + s16(FF2C00)+s16(FF2C04)+s16(FF2BFC)+s16(FF2A60)
y = 32*s8(record03-cameraY) + s8(record05+record06+animationDY)
    + s16(FF2C02)+s16(FF2C06)+s16(FF2BFE)+4
```

Byte additions wrap before sign extension. These coordinates exclude the hardware SAT bias of 128. They are not terrain top-left anchors. The native renderer does not rectangularly clip individual actor bodies in this path; foreground planes can obscure them.

Each component is 32x32 pixels, sixteen Genesis tiles in column-major tile order. Frame bits 11/12 flip the entire component; bits13/14 select palette, bit15 is priority. FB has two independent frame words and a displacement selector: 1=(0,-32), 2=(32,0), 3=(0,32), 4=(-32,0). The second component receives the same palette decision, but its own frame attributes.

## Animation and pattern streams

`0152F6` initializes a visual entry. Graphics type mapping is ROM `15516[type-1]`; pointer root is BE32 ROM `78810`. Animation mapping is ROM `154BA[type-1]`; pointer root is BE32 ROM `78814`. For states 0..3 the animation base is BE32 ROM `78804`; otherwise select the mapped animation base and subtract four from state. Entry offset is `8*state + (record01&FE)`; its BE16 displacement is relative to the animation base.

`01539C..0154B8` replays the script with elapsed record08. Normal frames are BE16 frame plus byte duration. Duration is signed-clamped to at least one. Frame is selected when elapsed < duration; otherwise subtract duration and continue. Opcodes:

| Byte | Operands / effect |
|---|---|
|FF|signed relative displacement, repeat count; native decrement/zero loop semantics|
|FE|signed relative displacement|
|FD|one metadata byte; immutable decoder consumes without playing sound|
|FC|align pointer upward to even, then read absolute BE32 script pointer|
|FB|selector, BE16 first frame, BE16 second frame, duration|
|FA|signed X offset|
|F9|signed Y offset|
|F8|metadata event; immutable decoder consumes no operands|

Other F0..F7 commands are unverified and rejected. Native dispatch's verified command table covers F8..FF only.

Frame bit8 chooses shared graphics root BE32 ROM `78800`; otherwise use the type-selected graphics pointer. Frame low byte indexes a BE32 absolute compressed-stream pointer in either table. This differs from DB1's relative graphics offsets. `FF11` dispatches to ROM `000EAE`: stream has BE16 decompressed length, one skipped byte, then tokens. A token <128 copies token+1 literal bytes; otherwise copy (token&127)+1 bytes from already-produced output at backward distance nextByte+1, permitting overlap. All verified body streams have length512. The implementation requires exactly512, checks every ROM access, rejects backwards references before output and output overruns, and limits animation execution to4096 operations.

`014F52..01501E` chooses palettes from state/type/visual flags and ROM `15572[type-1]`; frame attributes are retained. `014B64..014D30` modifies patterns for record11 bits7/6 using masks already generated in RAM, never invoking guest RNG. The immutable implementation reuses those RAM masks.

## SAT / UI contract

`014DB2` writes body/marker rows into RAM `FF2852` (A5+22E4). Markers are interleaved before each actor's body, selected by record28 and the six entries at ROM `014F22`; their tile attributes are separate from the body's dynamic cache. Pseudo effects use visible-list ID FF and entries at `FFBCE6` and must remain native effects. Cursor and selection rows live before the body list, at A5+2214 and +22DC. `015648` skips zero attributes, stops at FFFF, and compacts rows into actual SAT entries while adding128 to X/Y. Consequently static SAT indices cannot classify bodies.

`db2_actor_native_sat_body` walks the bounded live visual-cache chain at `FF20B2`, recognizes only non-FF entries with nonnegative pattern words, and compares dynamic first/second component tile starts. It does not classify markers or pseudo effects as bodies. Consumers can preserve these native rows while replacing body rendering; priority must still interact with foreground planes.

## Validation and limitations

`tests/db2_actors_test.c` covers pure synthetic absolute graphics pointers, LZ overlap, flight/depth, FB components, FA/F9 offsets, bounds, invalid types/IDs/camera, loops and SAT classification. GCC with `-std=c11 -Wall -Wextra -Werror` passes. No ROM bytes are embedded in tests.

Private `local/db2-wide/actor-realcheck.c` reads the protected ROM and base dump, decodes all 15 active records, and compares resident nonnegative native cache patterns whose frame agrees. All five comparable 512-byte patterns match VRAM exactly. Elapsed replay alone agrees with fourteen of fifteen cached frame words: actor149 is cached at0010 while elapsed replay selects0012. `db2_actor_decode_cached` therefore reads the live cache frame when chain membership, actor ID, type, state, direction and record pointer all agree. It reconstructs patterns from ROM, including second frames and cached FA/F9 offsets, without sampling VRAM. With this override all fifteen cached frame words agree; the five comparable resident patterns remain exact. `db2_actors_visible` prefers this override and falls back to record elapsed for all other active records. It also obtains equal-depth ordering from the entire bounded 72-node native list, rather than the first40 cache; actor-ID order is only the deterministic fallback for actors absent from that list.

The normal-game visual cache layout, verified from `0146C0..0154B8`:

| Hex offset | Meaning |
|---|---|
|00|next cache byte offset; FFFF terminates chain rooted at FF20B2|
|02|first frame word; FFFF uninitialized|
|04|first pattern index relative to base tile037E; negative suppresses native body|
|06|second frame word|
|08|second pattern index|
|0A|actorID<<8; FFxx identifies pseudo effect|
|0C|graphics pointer selected through78810/type map|
|10|current animation script pointer|
|14|cached type|
|15|remaining duration byte|
|16|repeat-loop byte|
|17|zero=one component; 1..4=second-component selector|
|18|cached state|
|19|cached direction|
|1A|absolute actor record pointer|
|1E,1F|signed animation X,Y offsets|

`0181F2` increments record08 for every active record processed by the 255-record loop. Thus uncached actors continue animating during normal simulation without a host clock or guest mutation. `017A7C` increments global simulation clock FFE066 and calls the actor updater; `004F0C` invokes the visual updater once later in that gameplay loop. Native pause bit0 at FF9C8D bypasses the simulation update; visual flag bit4 at FF2A89 separately bypasses cached animation advancement (`0148A8`). Any future host-only paused-animation policy must account for that distinction. No host-side temporal state is needed for normal gameplay, and the decoder remains immutable. The elapsed byte wraps at256 exactly as guest record08 does; no claim is made that an uncached actor possesses a native cache history that the game never maintained.

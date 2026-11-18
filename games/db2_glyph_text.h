#pragma once
/* English replacement for 16x16 glyph-ID strings (story/dialogue pools).
 * At the glyph renderer (ROM 05B0FE) each cell of a catalogued string gets a
 * host-built 1bpp bitmap holding two native 8x8 Latin letters. Pure
 * presentation: only the renderer's scratch glyph buffer is written. */
void db2_glyph_text_observe(unsigned pc);
void db2_glyph_text_reset(void);
/* Test hook: build the 32-byte 1bpp cell bitmap for two characters. */
int db2_glyph_text_cell(char left,char right,unsigned char out[32]);

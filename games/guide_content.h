#ifndef DYNA_GUIDE_CONTENT_H
#define DYNA_GUIDE_CONTENT_H
/* Pure immutable English content. Game IDs: 1=DB1, 2=DB2 Special.
 * No guest state reads/writes, captures, filesystem or renderer dependencies. */
enum { DYNA_GUIDE_UNITS, DYNA_GUIDE_HUD, DYNA_GUIDE_BASIC, DYNA_GUIDE_ADVANCED, DYNA_GUIDE_SECTIONS };
typedef struct DynaGuidePage {
 const char *title;
 const char *body; /* Newline-delimited, concise paragraphs. */
 const char *example;
 const char *capture_key; /* Stable key; existence does NOT mean captured. */
 const char *capture_scenario; /* Actual engine scene required, never an illustration. */
 const char *evidence; /* Evidence record anchor in docs/research/game-guide-evidence.md. */
} DynaGuidePage;
const char *dyna_guide_section_name(unsigned section);
unsigned dyna_guide_page_count(unsigned game,unsigned section);
const DynaGuidePage *dyna_guide_page(unsigned game,unsigned section,unsigned page);
/* Saturating navigation, robust against invalid persisted page indices. */
unsigned dyna_guide_move(unsigned game,unsigned section,unsigned page,int delta);
#endif

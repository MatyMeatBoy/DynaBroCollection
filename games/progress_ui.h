#ifndef DYNA_PROGRESS_UI_H
#define DYNA_PROGRESS_UI_H
#include <stdint.h>
enum { DYNA_PROGRESS_UI_UP=1, DYNA_PROGRESS_UI_DOWN=2,
 DYNA_PROGRESS_UI_CONFIRM=4, DYNA_PROGRESS_UI_START=8 };
typedef struct {
 unsigned candidate,active,selected,released,previous,continuing,pulse,exiting,kind;
 const char *feedback;
} DynaProgressUi;
void dyna_progress_ui_reset(DynaProgressUi *);
/* qualified_result means a validated display candidate from this hook's
 * expected source:1BECC ordinary result or28F1A timed hard-ending message. */
void dyna_progress_ui_before(DynaProgressUi *,uint32_t pc,int qualified_result);
/* Returns a single Back to Title request edge. Candidate/active override is1; start is a
 * bounded physical START request, never a CPU control-flow operation. */
int dyna_progress_ui_frame(DynaProgressUi *,unsigned buttons,int *override,int *start);
void dyna_progress_ui_saved(DynaProgressUi *,int success);
int dyna_progress_ui_prompt(const DynaProgressUi *,unsigned *,const char **);
int dyna_progress_ui_kind(const DynaProgressUi *);
#endif

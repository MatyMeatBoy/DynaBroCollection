#ifndef DB1_PASSWORD_NAV_H
#define DB1_PASSWORD_NAV_H
#include <stdint.h>
/* Abstract physical buttons. A/B/C match progress_replay.h; caller translates. */
enum { DB1_PASSWORD_A=1,DB1_PASSWORD_B=2,DB1_PASSWORD_C=4,
 DB1_PASSWORD_UP=8,DB1_PASSWORD_DOWN=16,DB1_PASSWORD_OTHER=32 };
typedef enum { DB1_PASSWORD_IDLE,DB1_PASSWORD_RELEASE,DB1_PASSWORD_MOVE,
 DB1_PASSWORD_CONFIRM,DB1_PASSWORD_WAIT,DB1_PASSWORD_HANDED_OFF,
 DB1_PASSWORD_CANCELED,DB1_PASSWORD_TIMEOUT,DB1_PASSWORD_UNAVAILABLE } Db1PasswordNavStatus;
typedef struct {
 Db1PasswordNavStatus status;
 unsigned native_selection,anchor,target,button,confirm,pulse,ack,elapsed,drain;
} Db1PasswordNav;
void db1_password_nav_reset(Db1PasswordNav *);
/* Mature getter and native anchor are supplied read-only by the host. */
int db1_password_nav_request(Db1PasswordNav *,int mature,unsigned native_selection);
/* physical_confirm must be exactly A/B/C, decoded from native RAM0028..002A.
 * Invalid mappings never emit confirmation. Observe before replay_before so
 * 1D7B6 hands off before the existing password hooks. */
void db1_password_nav_before(Db1PasswordNav *,uint32_t pc,unsigned native_selection,unsigned physical_confirm);
/* Once per guest frame. All real input after initial release cancels.
 * Cancel/timeout enters a release guard: owns remains true until all physical
 * input is released. Output pads never contain user input. */
void db1_password_nav_frame(Db1PasswordNav *,unsigned user_input,int *override,unsigned *pad);
void db1_password_nav_cancel(Db1PasswordNav *);
int db1_password_nav_owns(const Db1PasswordNav *);
Db1PasswordNavStatus db1_password_nav_status(const Db1PasswordNav *);
const char *db1_password_nav_status_text(const Db1PasswordNav *);
#endif

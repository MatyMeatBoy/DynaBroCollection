#ifndef DYNA_VIEWPORT_H
#define DYNA_VIEWPORT_H
typedef struct { int enabled,canvas,adaptive; } DynaViewport;
#define DYNA_VIEWPORT_DEFAULT {1,398,0}
/* Invalid modes leave the previous configuration untouched. */
int dyna_viewport_configure(DynaViewport *,const char *mode);
int dyna_viewport_width(const DynaViewport *,int drawable_w,int drawable_h,int native_h);
#endif

#pragma once
/* Pure physical-pixel layout; DPI is used only to choose readable sizing. */
typedef struct {int x,y,w,h;} DynaHubRect;
typedef struct {
 DynaHubRect banner,version,cards[2],settings[2],note,status;
 int ui_scale_milli;
} DynaHubLayout;
void dyna_hub_window_bounds(DynaHubRect work,unsigned dpi,int frame_w,int frame_h,
                           int minimum,DynaHubRect *out);
void dyna_hub_layout(int width,int height,unsigned dpi,DynaHubLayout *out);

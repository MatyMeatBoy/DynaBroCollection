/* Synthetic2x1 PNG transparency fixtures; no cartridge data. */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "stb_image.h"
#include <assert.h>
#include <string.h>
static const unsigned char png_0[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,1,0,0,0,105,245,71,51,0,0,0,2,116,82,78,83,0,42,173,40,4,238,0,0,0,11,73,68,65,84,120,156,99,96,128,2,0,0,9,0,1,251,82,184,169,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char png_1[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,255,0,0,0,15,76,38,167,0,0,0,2,116,82,78,83,0,42,173,40,4,238,0,0,0,11,73,68,65,84,120,156,99,96,128,2,0,0,9,0,1,251,82,184,169,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char png_2[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,5,0,0,0,230,151,208,100,0,0,0,2,116,82,78,83,0,42,173,40,4,238,0,0,0,11,73,68,65,84,120,156,99,96,128,2,0,0,9,0,1,251,82,184,169,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char png_3[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,7,0,0,0,76,158,24,239,0,0,0,2,116,82,78,83,0,42,173,40,4,238,0,0,0,11,73,68,65,84,120,156,99,96,128,2,0,0,9,0,1,251,82,184,169,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char png_4[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,3,0,0,0,0,166,153,17,71,0,0,0,2,116,82,78,83,0,42,173,40,4,238,0,0,0,11,73,68,65,84,120,156,99,96,96,0,0,0,3,0,1,184,173,58,99,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char png_5[]={137,80,78,71,13,10,26,10,0,0,0,12,73,72,68,82,0,0,0,2,0,0,0,1,8,0,0,0,253,45,215,130,0,0,0,2,116,82,78,83,0,42,173,40,4,238,0,0,0,11,73,68,65,84,120,156,99,96,96,0,0,0,3,0,1,184,173,58,99,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char png_6[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,2,0,0,0,123,64,232,221,0,0,0,2,116,82,78,83,0,42,173,40,4,238,0,0,0,11,73,68,65,84,120,156,99,96,0,3,0,0,7,0,1,178,134,172,244,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char png_7[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,0,0,0,0,1,8,0,0,0,0,213,188,240,107,0,0,0,2,116,82,78,83,0,42,173,40,4,238,0,0,0,11,73,68,65,84,120,156,99,96,96,0,0,0,3,0,1,184,173,58,99,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char png_8[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,16,0,0,0,0,129,217,252,21,0,0,0,2,116,82,78,83,18,52,47,211,73,94,0,0,0,13,73,68,65,84,120,156,99,16,50,9,171,0,0,2,13,1,21,169,126,165,198,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char png_9[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,0,0,0,0,209,73,32,86,0,0,0,2,116,82,78,83,0,42,173,40,4,238,0,0,0,11,73,68,65,84,120,156,99,208,74,6,0,0,186,0,142,100,71,50,37,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char png_10[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,4,0,0,0,94,43,183,1,0,0,0,4,116,82,78,83,0,42,0,42,93,242,108,122,0,0,0,13,73,68,65,84,120,156,99,208,250,159,252,31,0,5,111,2,140,91,185,143,226,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char png_11[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,16,2,0,0,0,43,208,52,158,0,0,0,6,116,82,78,83,18,52,86,120,154,188,137,228,78,230,0,0,0,21,73,68,65,84,120,156,99,16,50,9,171,152,181,231,222,7,65,37,99,23,0,31,225,4,227,96,198,83,91,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char png_12[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,2,0,0,0,123,64,232,221,0,0,0,6,116,82,78,83,0,12,0,34,0,56,184,157,192,128,0,0,0,15,73,68,65,84,120,156,99,224,81,178,136,138,142,1,0,3,249,1,120,19,246,219,9,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char png_13[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,6,0,0,0,244,34,127,138,0,0,0,8,116,82,78,83,0,12,0,12,0,12,0,12,181,27,96,155,0,0,0,17,73,68,65,84,120,156,99,224,81,178,248,31,21,29,243,31,0,11,210,3,118,118,209,55,85,0,0,0,0,73,69,78,68,174,66,96,130};
static const unsigned char png_14[]={137,80,78,71,13,10,26,10,0,0,0,2,116,82,78,83,0,42,173,40,4,238,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,0,0,0,0,209,73,32,86,0,0,0,11,73,68,65,84,120,156,99,96,96,0,0,0,3,0,1,184,173,58,99,0,0,0,0,73,69,78,68,174,66,96,130};
int main(void){
{ int x=0,y=0,n=0; unsigned char *pixels=stbi_load_from_memory(png_0,sizeof png_0,&x,&y,&n,4);
assert(!pixels);
}
{ int x=0,y=0,n=0; unsigned char *pixels=stbi_load_from_memory(png_1,sizeof png_1,&x,&y,&n,4);
assert(!pixels);
}
{ int x=0,y=0,n=0; unsigned char *pixels=stbi_load_from_memory(png_2,sizeof png_2,&x,&y,&n,4);
assert(!pixels);
}
{ int x=0,y=0,n=0; unsigned char *pixels=stbi_load_from_memory(png_3,sizeof png_3,&x,&y,&n,4);
assert(!pixels);
}
{ int x=0,y=0,n=0; unsigned char *pixels=stbi_load_from_memory(png_4,sizeof png_4,&x,&y,&n,4);
assert(!pixels);
}
{ int x=0,y=0,n=0; unsigned char *pixels=stbi_load_from_memory(png_5,sizeof png_5,&x,&y,&n,4);
assert(!pixels);
}
{ int x=0,y=0,n=0; unsigned char *pixels=stbi_load_from_memory(png_6,sizeof png_6,&x,&y,&n,4);
assert(!pixels);
}
{ int x=0,y=0,n=0; unsigned char *pixels=stbi_load_from_memory(png_7,sizeof png_7,&x,&y,&n,4);
assert(!pixels);
}
{ int x=0,y=0,n=0; unsigned char *pixels=stbi_load_from_memory(png_8,sizeof png_8,&x,&y,&n,4);
const unsigned char expected[]={18,18,18,0,86,86,86,255};
assert(pixels && x==2 && y==1 && n==2);assert(!memcmp(pixels,expected,sizeof expected));stbi_image_free(pixels);
}
{ int x=0,y=0,n=0; unsigned char *pixels=stbi_load_from_memory(png_9,sizeof png_9,&x,&y,&n,4);
const unsigned char expected[]={42,42,42,0,99,99,99,255};
assert(pixels && x==2 && y==1 && n==2);assert(!memcmp(pixels,expected,sizeof expected));stbi_image_free(pixels);
}
{ int x=0,y=0,n=0; unsigned char *pixels=stbi_load_from_memory(png_10,sizeof png_10,&x,&y,&n,4);
assert(!pixels);
}
{ int x=0,y=0,n=0; unsigned char *pixels=stbi_load_from_memory(png_11,sizeof png_11,&x,&y,&n,4);
const unsigned char expected[]={18,86,154,0,222,17,51,255};
assert(pixels && x==2 && y==1 && n==4);assert(!memcmp(pixels,expected,sizeof expected));stbi_image_free(pixels);
}
{ int x=0,y=0,n=0; unsigned char *pixels=stbi_load_from_memory(png_12,sizeof png_12,&x,&y,&n,4);
const unsigned char expected[]={12,34,56,0,90,91,92,255};
assert(pixels && x==2 && y==1 && n==4);assert(!memcmp(pixels,expected,sizeof expected));stbi_image_free(pixels);
}
{ int x=0,y=0,n=0; unsigned char *pixels=stbi_load_from_memory(png_13,sizeof png_13,&x,&y,&n,4);
assert(!pixels);
}
{ int x=0,y=0,n=0; unsigned char *pixels=stbi_load_from_memory(png_14,sizeof png_14,&x,&y,&n,4);
assert(!pixels);
}
return 0;}

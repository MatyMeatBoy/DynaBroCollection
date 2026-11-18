#include "guide_content.h"
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
int main(void){
 unsigned g,s,p;unsigned pages=0;
 for(g=1;g<=2;g++)for(s=0;s<DYNA_GUIDE_SECTIONS;s++){
  unsigned n=dyna_guide_page_count(g,s);assert(n&&n<=5);assert(dyna_guide_section_name(s));
  if(s==DYNA_GUIDE_BASIC||s==DYNA_GUIDE_ADVANCED)assert(n==2);
  for(p=0;p<n;p++){
   const DynaGuidePage *v=dyna_guide_page(g,s,p);assert(v&&v->title[0]&&v->body[0]&&v->example[0]);
   assert(v->capture_key[0]&&v->capture_scenario[0]&&v->evidence[0]);
   assert(strlen(v->body)<700);pages++;
   assert(dyna_guide_move(g,s,p,INT_MIN)==0);
   assert(dyna_guide_move(g,s,p,INT_MAX)==n-1);
   assert(dyna_guide_move(g,s,p,0)==p);
  }
  assert(!dyna_guide_page(g,s,n));assert(!dyna_guide_page(g,s,UINT_MAX));
  assert(dyna_guide_move(g,s,UINT_MAX,0)==n-1);
  assert(dyna_guide_move(g,s,0,-1)==0);assert(dyna_guide_move(g,s,n-1,1)==n-1);
 }
 assert(pages==17);assert(!dyna_guide_section_name(UINT_MAX));
 assert(!dyna_guide_page(0,0,0));assert(!dyna_guide_page(3,0,0));
 assert(!dyna_guide_page(1,UINT_MAX,0));assert(!dyna_guide_page_count(1,UINT_MAX));
 assert(!dyna_guide_move(0,0,UINT_MAX,INT_MIN));return 0;
}

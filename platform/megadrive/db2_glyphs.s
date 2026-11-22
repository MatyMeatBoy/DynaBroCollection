        org $25d300

; Replaces the first six bytes of the cell renderer at $05b124.
; A1 is the current glyph-word pointer. The translated cells enter the
; original expand/upload path at $05b178 with a 1bpp glyph in A0.
glyph_hook:
        movem.l d0-d4/a2-a4,-(sp)
        move.l  a1,d0
        andi.l  #$ffffff,d0
        cmpi.l  #$ff0000,d0
        bhs.w   ram_glyph

        lea     glyph_ranges,a2
        moveq   #0,d2
        move.w  #glyph_range_count,d3
; Upper bound over the sorted addresses. D2 is inclusive, D3 exclusive.
glyph_search:
        cmp.w   d3,d2
        bhs.s   glyph_search_done
        move.w  d2,d4
        add.w   d3,d4
        lsr.w   #1,d4
        move.w  d4,d1
        mulu.w  #12,d1
        cmp.l   (a2,d1.w),d0
        blo.s   glyph_search_before
        move.w  d4,d2
        addq.w  #1,d2
        bra.s   glyph_search
glyph_search_before:
        move.w  d4,d3
        bra.s   glyph_search

glyph_search_done:
        subq.w  #1,d2
        bmi.w   glyph_fallback
        mulu.w  #12,d2
        lea     (a2,d2.w),a2
        cmp.l   4(a2),d0
        bhs.w   glyph_fallback
        sub.l   (a2),d0
        lsr.l   #1,d0
        movea.l 8(a2),a3
        suba.l  a4,a4
glyph_render:
        move.w  $c(a6),d1
        lsr.w   #2,d1
        beq.w   glyph_fallback
        divu.w  d1,d0
        move.w  (a3)+,d2
        move.w  d2,d3
        andi.w  #$7fff,d3
        cmp.w   d3,d0
        bhs.w   glyph_fallback
        move.w  d0,d1
        lsl.w   #2,d1
        movea.l (a3,d1.w),a3
        swap    d0
        tst.w   d2
        bmi.w   glyph_bitmap
        add.w   d0,d0

        lea     font_ascii,a2
        moveq   #0,d4
        move.b  (a3)+,d4
        cmp.w   d4,d0
        bhs.s   glyph_blank_both
        moveq   #0,d1
        move.b  (a3,d0.w),d1
        bsr.w   replace_digit
        lsl.w   #3,d1
        move.w  d1,-(sp)
        addq.w  #1,d0
        cmp.w   d4,d0
        bhs.s   glyph_left_only
        moveq   #0,d1
        move.b  (a3,d0.w),d1
        bsr.w   replace_digit
        lsl.w   #3,d1
        move.w  (sp)+,d0
        lea     (a2,d0.w),a4
        adda.w  d1,a2
        bra.s   glyph_blank
glyph_left_only:
        move.w  (sp)+,d0
        lea     (a2,d0.w),a4
        bra.s   glyph_blank
glyph_blank_both:
        movea.l a2,a4

glyph_blank:
        bsr.s   glyph_destination
        movea.l a0,a3
        clr.l   (a3)+
        clr.l   (a3)+
        moveq   #7,d0
glyph_rows:
        move.b  (a4)+,(a3)+
        move.b  (a2)+,(a3)+
        dbra    d0,glyph_rows
        clr.l   (a3)+
        clr.l   (a3)
        bra.s   glyph_ready

glyph_destination:
        move.w  (a1),d0
        andi.w  #$f,d0
        lsl.w   #5,d0
        lea     $ffec66.l,a0
        adda.w  d0,a0
        rts

; Narrow biography fields have three five-pixel letters per cell.
glyph_bitmap:
        cmp.w   (a3)+,d0
        bhs.s   glyph_bitmap_clear
        lsl.w   #5,d0
        adda.w  d0,a3
        bra.s   glyph_bitmap_copy
glyph_bitmap_clear:
        lea     font_ascii,a3
glyph_bitmap_copy:
        bsr.s   glyph_destination
        movea.l a0,a4
        moveq   #7,d0
glyph_bitmap_loop:
        move.l  (a3)+,(a4)+
        dbra    d0,glyph_bitmap_loop
glyph_ready:
        movem.l (sp)+,d0-d4/a2-a4
        jmp     $05b178.l

glyph_fallback:
        movem.l (sp)+,d0-d4/a2-a4
        move.w  (a1),d0
        cmpi.w  #1,d0
        jmp     $05b12a.l

; Replace tutorial stage markers using the digit in the matched RAM stream.
; A4 is the source string here, or zero for a fixed ROM string.
replace_digit:
        cmpi.b  #35,d1
        bne.s   replace_done
        cmpa.l  #0,a4
        beq.s   replace_done
        move.w  12(a4),d1
        subi.w  #$cc,d1
        cmpi.w  #18,d1
        bhi.s   replace_space
        lsr.w   #1,d1
        bcs.s   replace_space
        addi.w  #48,d1
replace_done:
        rts
replace_space:
        moveq   #32,d1
        rts

; Match an original decompressed string once per renderer call. The original
; LINK reserves two new local longs, and the native UNLK frees them.
ram_glyph:
        tst.w   d5
        bne.w   ram_cached
        clr.l   -4(a6)
        movea.l d0,a4                   ; candidate beginning
        cmpa.l  #$ffffe4,a4              ; stop before scratch-address rollover
        bhi.w   glyph_fallback
        moveq   #63,d4
ram_previous:
        cmpa.l  #$ff0002,a4
        blo.w   glyph_fallback
        move.w  (a4),d2
        cmpi.w  #2,d2
        bne.s   ram_bucket
        moveq   #1,d2
ram_bucket:
        andi.w  #255,d2
        lsl.w   #2,d2
        lea     ram_buckets,a3
        movea.l (a3,d2.w),a3
        cmpa.l  #0,a3
        beq.s   ram_candidate_failed
ram_candidate:
        movea.l (a3)+,a2
        cmpa.l  #0,a2
        beq.s   ram_candidate_failed
        ; The first word is part of the bucket hash and still compared.
        moveq   #0,d3
ram_prefix:
        move.w  (a2,d3.w),d1
        cmpi.w  #$ffff,d1
        beq.s   ram_match
        move.w  (a4,d3.w),d2
        cmpi.w  #2,d2
        bne.s   ram_compare
        moveq   #1,d2
ram_compare:
        cmp.w   d1,d2
        bne.s   ram_candidate
        addq.w  #2,d3
        cmpi.w  #12,d3
        blo.s   ram_prefix
ram_match:
        move.l  d0,d1
        sub.l   a4,d1
        lsr.l   #1,d1
        cmp.w   12(a2),d1
        bhs.s   ram_candidate
        move.l  a2,-4(a6)
        move.l  a4,-8(a6)
        moveq   #0,d3
        move.w  12(a2),d3
        move.l  d3,d2
        add.l   d2,d2
        add.l   a4,d2
        cmpi.l  #$fffffc,d2
        bhi.s   ram_load_record
        movea.l a4,a3
        subq.w  #1,d3
ram_blank_loop:
        cmpi.w  #1,(a3)
        bne.s   ram_blank_next
        move.w  #2,(a3)
ram_blank_next:
        addq.l  #2,a3
        dbra    d3,ram_blank_loop
        bra.s   ram_load_record
ram_candidate_failed:
        subq.l  #2,a4
        dbra    d4,ram_previous
        bra.w   glyph_fallback
ram_cached:
        movea.l -4(a6),a2
        cmpa.l  #0,a2
        beq.w   glyph_fallback
        movea.l -8(a6),a4
ram_load_record:
        sub.l   a4,d0
        lsr.l   #1,d0
        cmp.w   12(a2),d0
        bhs.w   glyph_fallback
        sub.w   14(a2),d0
        blo.w   glyph_fallback
        lea     16(a2),a3
        bra.w   glyph_render

        include "rom-glyph-data.inc"

#include "manual_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const char *p; } Cursor;

static void skip_ws(Cursor *c) { while (*c->p == ' ' || *c->p == '\t' || *c->p == '\n' || *c->p == '\r') c->p++; }

/* Decodes one JSON string literal starting at '"'. Returns a malloc'd,
 * NUL-terminated copy of its contents (never longer than the source), or
 * NULL on malformed input. Advances *c past the closing quote. */
static char *parse_string(Cursor *c) {
    if (*c->p != '"') return NULL;
    c->p++;
    const char *start = c->p;
    size_t len = 0;
    for (const char *s = start; *s && *s != '"'; s++) { if (*s == '\\' && s[1]) s++; len++; }
    char *out = malloc(len + 1);
    if (!out) return NULL;
    size_t w = 0;
    while (*c->p && *c->p != '"') {
        if (*c->p == '\\' && c->p[1]) {
            c->p++;
            switch (*c->p) {
            case 'n': out[w++] = '\n'; break;
            case 't': out[w++] = '\t'; break;
            case 'r': out[w++] = '\r'; break;
            case '"': out[w++] = '"'; break;
            case '\\': out[w++] = '\\'; break;
            case '/': out[w++] = '/'; break;
            default: out[w++] = *c->p; break; /* \uXXXX and other escapes pass through unescaped */
            }
            c->p++;
        } else out[w++] = *c->p++;
    }
    out[w] = 0;
    if (*c->p != '"') { free(out); return NULL; }
    c->p++;
    return out;
}

static int parse_int(Cursor *c, int *out) {
    char *end;
    long v = strtol(c->p, &end, 10);
    if (end == c->p) return 0;
    c->p = end;
    *out = (int)v;
    return 1;
}

static int match(Cursor *c, const char *lit) {
    size_t n = strlen(lit);
    if (strncmp(c->p, lit, n) != 0) return 0;
    c->p += n;
    return 1;
}

static int parse_page(Cursor *c, DynaManualPage *page) {
    memset(page, 0, sizeof *page);
    skip_ws(c);
    if (*c->p != '{') return 0;
    c->p++;
    for (;;) {
        skip_ws(c);
        if (*c->p == '}') { c->p++; break; }
        char *key = parse_string(c);
        if (!key) return 0;
        skip_ws(c);
        if (*c->p != ':') { free(key); return 0; }
        c->p++;
        skip_ws(c);
        int ok = 1;
        if (!strcmp(key, "page")) {
            ok = parse_int(c, &page->page);
        } else if (!strcmp(key, "image")) {
            char *v = parse_string(c);
            if (!v) ok = 0; else { strncpy(page->image, v, sizeof page->image - 1); page->image[sizeof page->image - 1] = 0; free(v); }
        } else if (!strcmp(key, "title")) {
            if (match(c, "null")) page->title = NULL;
            else { page->title = parse_string(c); ok = page->title != NULL; }
        } else if (!strcmp(key, "translation")) {
            if (match(c, "null")) page->translation = NULL;
            else { page->translation = parse_string(c); ok = page->translation != NULL; }
        } else {
            /* Skip an unknown string value; this schema has no other field types. */
            char *v = parse_string(c);
            free(v); ok = v != NULL;
        }
        free(key);
        if (!ok) return 0;
        skip_ws(c);
        if (*c->p == ',') { c->p++; continue; }
        if (*c->p == '}') { c->p++; break; }
        return 0;
    }
    if (!page->translation) {
        page->translation = malloc(1);
        if (!page->translation) return 0;
        page->translation[0] = 0;
    }
    return 1;
}

int dyna_manual_load_text(const char *json_text, DynaManualBook *out) {
    out->pages = NULL; out->count = 0;
    if (!json_text) return 0;
    Cursor c = { json_text };
    skip_ws(&c);
    if (*c.p != '[') return 0;
    c.p++;
    size_t cap = 0;
    for (;;) {
        skip_ws(&c);
        if (*c.p == ']') { c.p++; break; }
        if (out->count > 0) {
            if (*c.p != ',') { dyna_manual_free(out); return 0; }
            c.p++;
            skip_ws(&c);
        }
        if (out->count >= cap) {
            cap = cap ? cap * 2 : 16;
            DynaManualPage *grown = realloc(out->pages, cap * sizeof *grown);
            if (!grown) { dyna_manual_free(out); return 0; }
            out->pages = grown;
        }
        if (!parse_page(&c, &out->pages[out->count])) { dyna_manual_free(out); return 0; }
        out->count++;
    }
    return 1;
}

int dyna_manual_load_file(const char *path, DynaManualBook *out) {
    out->pages = NULL; out->count = 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END)) { fclose(f); return 0; }
    long size = ftell(f);
    if (size < 0 || fseek(f, 0, SEEK_SET)) { fclose(f); return 0; }
    char *buffer = malloc((size_t)size + 1);
    if (!buffer) { fclose(f); return 0; }
    size_t read = fread(buffer, 1, (size_t)size, f);
    fclose(f);
    buffer[read] = 0;
    int ok = dyna_manual_load_text(buffer, out);
    free(buffer);
    return ok;
}

void dyna_manual_free(DynaManualBook *book) {
    for (size_t i = 0; i < book->count; i++) { free(book->pages[i].title); free(book->pages[i].translation); }
    free(book->pages);
    book->pages = NULL; book->count = 0;
}

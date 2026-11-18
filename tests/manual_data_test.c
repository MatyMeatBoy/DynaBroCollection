#include "manual_data.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

static void test_inline(void) {
    const char *json =
        "[\n"
        "  {\"page\":1,\"image\":\"a.jpg\",\"title\":\"Cover\",\"translation\":\"Line one\\nLine \\\"two\\\"\"},\n"
        "  {\"page\":2,\"image\":\"b.jpg\",\"title\":null,\"translation\":\"No title page.\"}\n"
        "]";
    DynaManualBook book;
    assert(dyna_manual_load_text(json, &book));
    assert(book.count == 2);
    assert(book.pages[0].page == 1);
    assert(!strcmp(book.pages[0].image, "a.jpg"));
    assert(!strcmp(book.pages[0].title, "Cover"));
    assert(!strcmp(book.pages[0].translation, "Line one\nLine \"two\""));
    assert(book.pages[1].page == 2);
    assert(book.pages[1].title == NULL);
    assert(!strcmp(book.pages[1].translation, "No title page."));
    dyna_manual_free(&book);
    assert(book.pages == NULL && book.count == 0);
}

static void test_malformed_rejected(void) {
    DynaManualBook book;
    assert(!dyna_manual_load_text("", &book));
    assert(book.count == 0);
    assert(!dyna_manual_load_text("[", &book));
    assert(!dyna_manual_load_text("[{\"page\":1}", &book));
    assert(!dyna_manual_load_text("not json", &book));
}

static void test_missing_file(void) {
    DynaManualBook book;
    assert(!dyna_manual_load_file("does/not/exist.json", &book));
    assert(book.pages == NULL && book.count == 0);
}

/* The real translation files this project ships; parse them end to end. */
static void test_real_file(const char *path, int expected_count) {
    DynaManualBook book;
    assert(dyna_manual_load_file(path, &book));
    assert((int)book.count == expected_count);
    for (size_t i = 0; i < book.count; i++) {
        assert(book.pages[i].page == (int)i + 1);
        assert(book.pages[i].image[0] != 0);
        assert(book.pages[i].translation != NULL);
    }
    dyna_manual_free(&book);
}

int main(int argc, char **argv) {
    test_inline();
    test_malformed_rejected();
    test_missing_file();
    if (argc >= 3) {
        test_real_file(argv[1], 44);
        test_real_file(argv[2], 27);
    } else {
        fprintf(stderr, "manual data: skipping real-file cases (expected <db1.json> <db2.json> argv)\n");
    }
    puts("manual data: inline/malformed/missing/real-file cases passed");
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <wchar.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <error.h>

#include "remix.h"

#undef CHECK_VALID_INPUT

void show_help(const char *name) {
    wprintf(L"Usage:\n");
    wprintf(L"\t%s [words list]\n", name);
}


wchar_t *char_to_wchar(const char *arg_c) {
    int length = strlen(arg_c);

    wchar_t *arg_w = (wchar_t *) malloc(length * sizeof(wchar_t));
    
    if (arg_w == NULL)
        return NULL;

    if (mbsrtowcs(arg_w, &arg_c, length, NULL) < 0)
        return NULL;

    return arg_w;
}


void process_word(const wchar_t *arg) {
    wchar_t **mixes;
    int mixes_size;

    wprintf(L"Mixes of %ls:\n", arg);

#ifdef CHECK_VALID_INPUT
    if (!is_valid_word(arg)) {
        wprintf(L"\tNot a valid word");
        return;
    }
#endif
    const struct remix_t options = { threads_number : 4, 
                                     allow_partial  : 1 };

    if ((mixes = do_remix(arg, wcslen(arg), &mixes_size, &options)) == NULL) {
        error(EXIT_FAILURE, errno, "Mixing letters failed");
    }

    assert(mixes_size >= 0);

    if (mixes_size == 0)
        wprintf(L"\tNone found\n");

    while (mixes_size > 0) {
        mixes_size--;
        wprintf(L"\t-> %ls\n", mixes[mixes_size]);
        free(mixes[mixes_size]);
    }

    free(mixes);
}


int main(int argc, const char **argv) {    
    if (setlocale(LC_ALL, "ru_RU.utf8") == NULL) {
        error(EXIT_FAILURE, 0, "Cannot set program locale");
    }

    if (fwide(stdout, 1) < 0) {
        error(EXIT_FAILURE, 0, "Cannot set stdout to wide-character IO mode");
    }

    if (argc <= 1) {
        show_help(argv[0]);
        return EXIT_SUCCESS;
    }
    
    while (argc > 1) {
        wchar_t *arg;
        if ((arg = char_to_wchar(argv[--argc])) == NULL) {
            perror(strerror(errno));
            return EXIT_FAILURE;
        }

        process_word(arg);

        free(arg);
    }

    return EXIT_SUCCESS;
}

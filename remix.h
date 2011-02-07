#ifndef _REMIX_H
#define _REMIX_H

#include <locale.h>
#include <stddef.h>

struct remix_t {
    int threads_number;
    int allow_partial;
};

wchar_t **do_remix(const wchar_t *word, int length, int *words_found, const struct remix_t *options);

int is_valid_word(const wchar_t *word);

#endif /* _REMIX_H */

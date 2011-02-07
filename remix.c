#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <wchar.h>

#include <pthread.h>
#include <search.h>

#include "remix.h"

//#undef DEBUG

extern wchar_t *wordlist[];
extern int wordlist_size;

/*
 *  Globals
 */

void *wordlist_tree = NULL;


/*
 *  Threads
 */

struct task_pool_t {
    wchar_t **tasks;
    int tasks_size;
    int first_unprocessed;    

    int word_length;

    wchar_t **results;
    int *results_size;

    pthread_mutex_t monitor;
};


struct thread_task_t {
    int thread_num;
    struct task_pool_t *pool;
};


typedef void * (*executor_t)(void *);

struct task_pool_t *task_pool_create(wchar_t **words, int words_size, int length, int *results_size);

wchar_t **process_pool(wchar_t **mixes, int mixes_size, int length, int *count, executor_t executor, int nthreads);


/*
 *  Common utils
 */

#define SHORT_FACTORS_SIZE  20

int initialized = 0;

static const unsigned long long factors[SHORT_FACTORS_SIZE] = {
    1,
    1,
    2,
    6,
    24,
    120,
    720,
    5040,
    40320,
    362880,
    3628800,
    39916800,
    479001600,
    6227020800ULL,
    87178291200ULL,
    1307674368000ULL,
    20922789888000ULL,
    355687428096000ULL,
    6402373705728000ULL,
    121645100408832000ULL
};

static unsigned long long fact(int n) {
    assert(n >= 0);

    if (n < 0) {
        return 0;
    }

    if (n < (sizeof(factors)/sizeof(unsigned long long)) ) {
        return factors[n];
    }

    unsigned long long result = 1;
    while (n != 0)
        result *= (n--);

    return result;
}


inline void swap(wchar_t *a, wchar_t *b) {
    wchar_t c;
    c = *a;
    *a = *b;
    *b = c;
}


/*
 *  Permutations
 */


wchar_t *sort(const wchar_t *word, int length) {
    int i, j;
    wchar_t *res;    

    assert(word != NULL);
    assert(length >= 0);

    if ((res = (wchar_t *) malloc((length+1) * sizeof(wchar_t))) == NULL) {
        return NULL;
    }

    memcpy(res, word, length * sizeof(wchar_t));

    for (i = 0; i < length; i++) {
        for (j = i + 1; j < length; j++) {
            if (res[i] > res[j])
                swap(&res[i], &res[j]);
        }
    }

    res[length] = L'\0';
    return res;
}


void remix_init() {
    int word_num;
    void *node;

    if (initialized) {
#ifdef DEBUG
        wprintf(L"Already initialized\n");
#endif
        return;
    }

#ifdef DEBUG
        wprintf(L"Initializing\n");
#endif

    for (word_num = 0; word_num < wordlist_size; word_num++) {
        node = tsearch((void *) wordlist[word_num], &wordlist_tree, (__compar_fn_t) wcscmp);
        assert(node != NULL);
    }
    
    initialized = 1;

#ifdef DEBUG
        wprintf(L"Initialized\n");
#endif

}


wchar_t *mix_next(wchar_t *next, int length) {
    assert(length >= 0);
    assert(next != NULL);

    int i, j;
    
    if (next == NULL) {
        return NULL;
    }
    
#ifdef DEBUG
    wprintf(L"IN: %ls\n", next);
#endif

    i = length - 1;
    while ( (i > 1) && (next[i]<=next[i-1]) ) {
        i--;
    }

/*#ifdef DEBUG
    wprintf(L"i = %i\n", i);
#endif*/

    j = length - 1;
    while (j>0 && next[j] <= next[i-1]) {
        j--;
    }

/*#ifdef DEBUG
    wprintf(L"j = %i\n", j);
#endif*/
    
    swap(&next[i-1], &next[j]);

    for (j = 0; j <= (length - i) / 2 - 1; j++) {
        assert( (i+j) < length);
        assert( (length - 1 - j) >= 0);
        swap(&next[i+j], &next[length - 1 - j]);
    }

    assert (next[length] == L'\0');

#ifdef DEBUG
    wprintf(L"OUT: %ls\n", next);
#endif

    return next;
}


inline int ptr_wcscmp(void *p1, void *p2) {
    wchar_t *s1 = ((wchar_t *) *( (wchar_t **)p1 ));
    wchar_t *s2 = ((wchar_t *) *( (wchar_t **)p2 ));

    return wcscmp(s1, s2);
}


inline wchar_t **remove_duplicates(wchar_t **all_mixes, int max_size, int length, int *count, int nthreads);

wchar_t **mix(const wchar_t *word, size_t length, int allow_partial, 
              int max_size, int *count, 
              int nthreads) {
    int nMix;

    wchar_t **all_mixes = (wchar_t **) calloc(max_size, sizeof(wchar_t *));
    if (all_mixes == NULL) {
        return NULL;
    }

    // .. that malloc will fail. Linked lists?
    wchar_t **result = (wchar_t **) malloc(max_size * sizeof(wchar_t *));
    if (result == NULL) {
        return NULL;
    }


    *count = 0;
    all_mixes[0] = sort(word, length);

    int part_length;

    if (allow_partial) {
        for (nMix = 1; nMix < max_size; nMix++) {
            int prev_mix = nMix - 1;
            for (part_length = 1; part_length <= length; part_length++, nMix++) {
                all_mixes[nMix] = (wchar_t *) malloc((part_length + 1) * sizeof(wchar_t));
                memcpy(all_mixes[nMix], all_mixes[prev_mix], (part_length) * sizeof(wchar_t));
                all_mixes[nMix][part_length] = L'\0';
            }
            nMix--;
            mix_next(all_mixes[nMix], length);
        }
    } else {
        for (nMix = 1; nMix < max_size; nMix++) {
            all_mixes[nMix] = (wchar_t *) malloc((length + 1) * sizeof(wchar_t));
            memcpy(all_mixes[nMix], all_mixes[nMix-1], (length + 1) * sizeof(wchar_t));
            mix_next(all_mixes[nMix], length);
        }
    }

#ifdef DEBUG
    for (nMix = 0; nMix < max_size; nMix++)
        wprintf(L"in: %i: %ls\n", nMix, all_mixes[nMix]);
#endif

    // remove duplicates
    result = remove_duplicates(all_mixes, max_size, length, count, nthreads);

#ifdef DEBUG
    for (nMix = 0; nMix < *count; nMix++)
        wprintf(L"%i: %ls\n", nMix, result[nMix]);
#endif
    return result;
}

static void *dupl_thread(void *arg) {
    struct thread_task_t *ttask = (struct thread_task_t *) arg;
    struct task_pool_t *pool = ttask->pool;
    wchar_t *word_task;

/*#ifdef DEBUG
        wprintf(L"Checking thread %i: started\n", ttask->thread_num);
#endif*/

    while (1) {
        pthread_mutex_lock(&(pool->monitor));
        if (pool->first_unprocessed >= pool->tasks_size) {
            pthread_mutex_unlock(&(pool->monitor));
            break;
        }
        word_task = pool->tasks[pool->first_unprocessed++];
        pthread_mutex_unlock(&(pool->monitor));

        if(!bsearch(&word_task, pool->results, *pool->results_size, sizeof(wchar_t *), (__compar_fn_t) &ptr_wcscmp)) {
            pthread_mutex_lock(&(pool->monitor));
            pool->results[(*(pool->results_size))++] = word_task;
            pthread_mutex_unlock(&(pool->monitor));
/*#ifdef DEBUG
            wprintf(L"Checking thread %i: GOOD: %ls\n", ttask->thread_num, word_task);
#endif */
        } else {
/*#ifdef DEBUG
            wprintf(L"Checking thread %i: BAD: %ls\n", ttask->thread_num, word_task);
#endif */
            free(word_task);
        }
    }

#ifdef DEBUG
        wprintf(L"Checking thread %i: exited\n", ttask->thread_num);
#endif

    return pool->results;
}


inline wchar_t **remove_duplicates(wchar_t **all_mixes, int max_size, int length, int *count, int nthreads){
    int nMix;
    qsort(all_mixes, max_size, sizeof(wchar_t *), (__compar_fn_t) &ptr_wcscmp);

    return process_pool(all_mixes, max_size, length, count, &dupl_thread, nthreads);
}


/*
 *  Word list access
 */


inline int is_valid_word(const wchar_t *word) {
    assert(word != NULL);
    return tfind((void *) word, &wordlist_tree, (__compar_fn_t) wcscmp) != NULL;
}


static void *remix_thread(void *arg) {
    struct thread_task_t *ttask = (struct thread_task_t *) arg;
    struct task_pool_t *pool = ttask->pool;
    wchar_t *word_task;

/*#ifdef DEBUG
        wprintf(L"Checking thread %i: started\n", ttask->thread_num);
#endif*/

    while (1) {
        pthread_mutex_lock(&(pool->monitor));
        if (pool->first_unprocessed >= pool->tasks_size) {
            pthread_mutex_unlock(&(pool->monitor));
            break;
        }
        word_task = pool->tasks[pool->first_unprocessed++];
        pthread_mutex_unlock(&(pool->monitor));
        
        if (is_valid_word(word_task)) {            
            pthread_mutex_lock(&(pool->monitor));
            pool->results[(*(pool->results_size))++] = word_task;
            pthread_mutex_unlock(&(pool->monitor));
/*#ifdef DEBUG
            wprintf(L"Checking thread %i: GOOD: %ls\n", ttask->thread_num, word_task);
#endif */
        } else {
/*#ifdef DEBUG
            wprintf(L"Checking thread %i: BAD: %ls\n", ttask->thread_num, word_task);
#endif */
            free(word_task);
        }
    }

#ifdef DEBUG
        wprintf(L"Checking thread %i: exited\n", ttask->thread_num);
#endif

    return NULL;
}


struct task_pool_t *task_pool_create(wchar_t **words, int words_size, int length, int *results_size) {
    assert(words_size >= 0);
    assert(length >= 0);

    struct task_pool_t *pool = (struct task_pool_t *) malloc(sizeof(struct task_pool_t));
    pool->word_length = length;

    if (pthread_mutex_init(&(pool->monitor), NULL) != 0) {
        return NULL;
    }
    
    pool->results_size = results_size;
    if ((pool->results = (wchar_t **) malloc(words_size * sizeof(wchar_t *))) == NULL) {
        return NULL;
    }

    pool->tasks_size = words_size;
    pool->tasks = words;
    pool->first_unprocessed = 0;
    memset(&(pool->tasks[words_size]), 0, sizeof(struct task_t *));

    return pool;
}


wchar_t **process_pool(wchar_t **mixes, int mixes_size, int length, int *count, executor_t executor, int nthreads) {
    // create task pool
    struct task_pool_t *task_pool = task_pool_create(mixes, mixes_size, length, count);
    if (task_pool == NULL) {
        return NULL;
    }

    // initialize threads
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) {
        return NULL;
    }

    pthread_t *threads = (pthread_t *) calloc(nthreads, sizeof(pthread_t));
    if (threads == NULL) {
        return NULL;
    }

    struct thread_task_t **thread_tasks = 
        (struct thread_task_t **) calloc(nthreads, sizeof(struct thread_task_t *));

    int tnum;
    for (tnum = 0; tnum < nthreads; tnum++) {
        struct thread_task_t *ttask = malloc(sizeof(struct thread_task_t));
        ttask->thread_num = tnum;
        ttask->pool = task_pool;
        thread_tasks[tnum] = ttask;
        if (pthread_create(&threads[tnum], &attr, executor, ttask) != 0) {
            return NULL;
        }
    }

    if (pthread_attr_destroy(&attr) != 0) {
        return NULL;
    }

    // collect results
    void *res;
    for (tnum = 0; tnum < nthreads; tnum++) {
        if (pthread_join(threads[tnum], &res)) {
            return NULL;
        }
        free(thread_tasks[tnum]);
    }
    free(thread_tasks);

    pthread_mutex_destroy(&(task_pool->monitor));

    free(task_pool->tasks);
    return task_pool->results;
}

wchar_t **do_remix(const wchar_t *word, int length, int *words_found, const struct remix_t *options) {
    assert(word != NULL);
    assert(words_found != NULL);
    assert(length >= 0);    

    remix_init();

    wchar_t **mixes;
    int mixes_size, mixes_max;
    int tnum;

    if (word == NULL || length == 0) {
        return NULL;
    }

    // mixes_max may be so huuuge ...
    mixes_max = fact(length) * ( options->allow_partial ? length : 1 );

    // get all mixes
    
    if ((mixes = mix(word, length, options->allow_partial, mixes_max, &mixes_size, 
                    options->threads_number)) == NULL) {
        return NULL;
    }

    assert(mixes_size >= 0);

#ifdef DEBUG
    wprintf(L"Starting search\n");
#endif

    return process_pool(mixes, mixes_size, length, words_found, &remix_thread, options->threads_number);
}


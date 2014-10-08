//
//  oxdeq.c
//
//  Created by Michael Henderson on 10/2/14.
//  Copyright (c) 2014 Michael D Henderson. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

// simple double ended queue. has no understanding of the
// data that it is storing. supports push/pop/peek operations
// to the front and back of the queue. supports reversing the
// data (which is odd because you could just push/pop from the
// other end of the queue to do the same thing).
//
typedef struct oxdeq {
    struct oxqent {
        struct oxqent *left;
        struct oxqent *right;
        void          *data;
    } *left, *right, *e, *t;
    size_t numberOfEntries;
} oxdeq;

oxdeq  *oxdeq_alloc(void);
oxdeq  *oxdeq_copy(oxdeq *q, void *(*fcopy)(void *data));
ssize_t oxdeq_number_of_entries(oxdeq *q);
void   *oxdeq_peek_left(oxdeq *q);
void   *oxdeq_peek_right(oxdeq *q);
void   *oxdeq_pop_left(oxdeq *q);
void   *oxdeq_pop_right(oxdeq *q);
oxdeq  *oxdeq_push_left(oxdeq *q, void *data);
oxdeq  *oxdeq_push_right(oxdeq *q, void *data);
oxdeq  *oxdeq_reverse(oxdeq *q);

// convenience macros to peek/push/pop like a stack
//
#define oxdeq_peek(q)    oxdeq_peek_left((q))
#define oxdeq_pop(q)     oxdeq_pop_left((q))
#define oxdeq_push(q, d) oxdeq_push_left((q), (d))

// simple list of values that can be easily extended.
//
typedef struct oxval {
    union {
        int           boolean;
        struct oxdeq *queue;
        struct {
            union {
                int    integer;
                double real;
            } value;
            int    isNull;
            enum { oxtInteger , oxtReal } kind;
        } number;
        struct {
            char   *value;
            ssize_t length;
            int     isNull;
        } text;
        struct {
            char         *name;
            struct oxval *value;
        } symbol;
    } u;
    enum { oxtBool, oxtNumber, oxtQueue, oxtSymbol, oxtText } kind;
    unsigned char data[1];
} oxval;

oxval *oxval_alloc(char type, char isNull, ...);

//-------------------------------------------------------------------

oxdeq *oxdeq_alloc(void) {
    oxdeq *q = malloc(sizeof(*q));
    if (!q) {
        perror(__FUNCTION__);
        exit(2);
    }
    q->left = q->right = q->e = q->t = 0;
    q->numberOfEntries = 0;
    return q;
}

// potentially create a deep copy of a queue.
//
// the 'fcopy' function is responsible for creating a deep copy
// of the data. if function is not supplied, then will do a shallow
// copy of the queue by sharing the data between the two queues.
//
// only the caller knows if that is desirable.
//
oxdeq *oxdeq_copy(oxdeq *q, void *(*fcopy)(void *data)) {
    oxdeq *c = oxdeq_alloc();
    if (q && q->numberOfEntries) {
        q->e = q->left;
        while (q->e) {
            void *copyOfData = fcopy ? fcopy(q->e->data) : q->e->data;
            oxdeq_push_right(c, copyOfData);
            q->e = q->e->right;
        }
    }
    return c;
}

ssize_t oxdeq_number_of_entries(oxdeq *q) {
    return q ? q->numberOfEntries : 0;
}

void *oxdeq_peek_left(oxdeq *q) {
    return (q && q->left) ? q->left->data : 0;
}

void *oxdeq_peek_right(oxdeq *q) {
    return (q && q->right) ? q->right->data : 0;
}

void *oxdeq_pop_left(oxdeq *q) {
    void *v = (q && q->left) ? q->left->data : 0;
    if (v) {
        q->e = q->left;
        if (!q->left->right) {
            q->left  = q->right = 0;
        } else {
            q->left  = q->left->right;
        }
        q->numberOfEntries--;
        free(q->e);
        q->e = 0;
    }
    return v;
}

void *oxdeq_pop_right(oxdeq *q) {
    void *v = (q && q->right) ? q->right->data : 0;
    if (v) {
        q->e = q->right;
        if (!q->right->left) {
            q->left  = q->right = 0;
        } else {
            q->right = q->right->left;
        }
        q->numberOfEntries--;
        free(q->e);
        q->e = 0;
    }
    return v;
}

oxdeq *oxdeq_push_left(oxdeq *q, void *data) {
    if (q && data) {
        q->e = malloc(sizeof(*(q->e)));
        if (!q->e) {
            perror(__FUNCTION__);
            exit(2);
        }
        q->e->data  = data;
        q->e->left  = 0;
        q->e->right = q->left;
        if (!q->left) {
            q->left = q->right      = q->e;
        } else {
            q->left = q->left->left = q->e;
        }
        q->numberOfEntries++;
        q->e = 0;
    }
    return q;
}

oxdeq *oxdeq_push_right(oxdeq *q, void *data) {
    if (q && data) {
        q->e = malloc(sizeof(*(q->e)));
        if (!q->e) {
            perror(__FUNCTION__);
            exit(2);
        }
        q->e->data  = data;
        q->e->left  = q->right;
        q->e->right = 0;
        if (!q->right) {
            q->left  = q->right        = q->e;
        } else {
            q->right = q->right->right = q->e;
        }
        q->numberOfEntries++;
        q->e = 0;
    }
    return q;
}

// reverse the queue by swapping the data in all of the entries.
// start at the front and the back and work until we meet in the
// middle.
//
oxdeq *oxdeq_reverse(oxdeq *q) {
    if (q) {
        q->e = q->left;
        q->t = q->right;

        while (q->e != q->t) {
            void *tmp  = q->e->data;
            q->e->data = q->t->data;
            q->t->data = tmp;
            
            if (q->e->right == q->t) {
                // the two ends have met, so we're done
                //
                break;
            }

            // haven't met, so move the ends towards the middle
            //
            q->e = q->e->right;
            q->t = q->t->left;
        }
        q->e = q->t = 0;
    }
    return q;
}

static oxval *oxval_alloc_boolean(int value);
static oxval *oxval_alloc_integer(int value, int isNull);
static oxval *oxval_alloc_queue(oxdeq *queue);
static oxval *oxval_alloc_real(double value, int isNull);
static oxval *oxval_alloc_symbol(const char *name, oxval *value);
static oxval *oxval_alloc_text(const char *value);

//
// oxval_alloc(flag, isNullFlag, value)
//
// flag value__ is_null______________________
//    b boolean not applicable
//    d real    per the isNull flag
//    i integer per the isNull flag
//    q queue   not applicable
//    s symbol  not applicable
//    t text    if ptr is null or isNull flag
//
oxval *oxval_alloc(char type, char isNull, ...) {
    oxval *v = malloc(sizeof(*v));
    if (!v) {
        perror(__FUNCTION__);
        exit(2);
    }
    
    int         boolean = 0;
    int         integer = 0;
    const char *name    = NULL;
    oxdeq      *queue   = NULL;
    double      real    = 0.0;
    const char *text    = NULL;
    oxval      *value   = NULL;
    
    va_list ap;
    va_start(ap, isNull);
    
    switch (type) {
        case 'b':
            boolean = va_arg(ap, int);
            break;
        case 'd':
            real = va_arg(ap, double);
            break;
        case 'f':
            real = va_arg(ap, double);
            break;
        case 'i':
            integer = va_arg(ap, int);
            break;
        case 'q':
            queue = va_arg(ap, oxdeq *);
            break;
        case 's':
            name  = va_arg(ap, const char *);
            value = va_arg(ap, oxval *);
            break;
        case 't':
            text = va_arg(ap, const char *);
            break;
        default:
            boolean = 0;
            integer = 0;
            name    = NULL;
            queue   = NULL;
            real    = 0.0;
            text    = NULL;
            value   = NULL;
            break;
    }
    
    va_end(ap);
    
    switch (type) {
        case 'b':
            return oxval_alloc_boolean(boolean);
        case 'd':
            return oxval_alloc_real(real, isNull);
        case 'f':
            return oxval_alloc_real(real, isNull);
        case 'i':
            return oxval_alloc_integer(integer, isNull);
        case 'q':
            return oxval_alloc_queue(queue);
        case 's':
            return oxval_alloc_symbol(name, value);
        case 't':
            return oxval_alloc_text(isNull ? 0 : text);
    }
    
    fprintf(stderr, "error:\tinvalid type '%c' passed to %s\n", type, __FUNCTION__);
    exit(2);
    
    // NOT REACHED
    return 0;
}

static oxval *oxval_alloc_boolean(int value) {
    oxval *v = malloc(sizeof(*v));
    if (!v) {
        perror(__FUNCTION__);
        exit(2);
    }
    v->kind      = oxtBool;
    v->u.boolean = value ? -1 : 0;
    return v;
}

static oxval *oxval_alloc_integer(int value, int isNull) {
    oxval *v = malloc(sizeof(*v));
    if (!v) {
        perror(__FUNCTION__);
        exit(2);
    }
    v->kind                   = oxtNumber;
    v->u.number.kind          = oxtInteger;
    v->u.number.isNull        = isNull ? -1 : 0;
    v->u.number.value.integer = value;
    return v;
}

static oxval *oxval_alloc_queue(oxdeq *queue) {
    oxval *v = malloc(sizeof(*v));
    if (!v) {
        perror(__FUNCTION__);
        exit(2);
    }
    v->kind      = oxtQueue;
    if (queue) {
        v->u.queue   = oxdeq_copy(queue, 0);
    } else {
        v->u.queue   = oxdeq_alloc();
    }
    if (!v->u.queue) {
        perror(__FUNCTION__);
        exit(2);
    }
    return v;
}

static oxval *oxval_alloc_real(double value, int isNull) {
    oxval *v = malloc(sizeof(*v));
    if (!v) {
        perror(__FUNCTION__);
        exit(2);
    }
    v->kind                = oxtNumber;
    v->u.number.kind       = oxtReal;
    v->u.number.isNull     = isNull ? -1 : 0;
    v->u.number.value.real = value;
    return v;
}

static oxval *oxval_alloc_symbol(const char *name, oxval *value) {
    ssize_t len = name ? strlen(name) : 0;
    oxval *v = malloc(sizeof(*v) + len);
    if (!v) {
        perror(__FUNCTION__);
        exit(2);
    }
    v->kind                = oxtSymbol;
    v->u.symbol.name       = (char *)(v->data);;
    v->u.symbol.value      = value;
    strcpy(v->u.symbol.name, name ? name : "");
    return v;
}

static oxval *oxval_alloc_text(const char *value) {
    ssize_t len = value ? strlen(value) : 0;
    oxval *v = malloc(sizeof(*v) + len);
    if (!v) {
        perror(__FUNCTION__);
        exit(2);
    }
    v->kind                   = oxtText;
    v->u.text.isNull          = value ? 0 : -1;
    v->u.text.length          = len;
    v->u.text.value           = (char *)(v->data);
    strcpy(v->u.text.value, value ? value : "");
    return v;
}

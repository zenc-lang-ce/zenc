#ifndef TEST_C_HEADER_TAGS_H
#define TEST_C_HEADER_TAGS_H

struct header_vec { int x; int y; };
union header_num { int i; float f; };
typedef struct {
    int a;
} header_anon;

#endif

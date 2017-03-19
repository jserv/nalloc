#include "nalloc.h"

struct matrix { size_t rows, cols; int **data; };

struct matrix *matrix_new(size_t rows, size_t cols)
{
    struct matrix *m = ncalloc(sizeof(*m), NULL);
    m->rows = rows; m->cols = cols;
    m->data = ncalloc(rows * sizeof(*m->data), m);
    for (size_t i = 0; i < rows; i++)
        m->data[i] = nalloc(cols * sizeof(**m->data), m->data);
     return m;
 }
 
void matrix_delete(struct matrix *m) { nfree(m); }

int main()
{
    struct matrix *m = matrix_new(4, 4);
    matrix_delete(m);
    return 0;
}

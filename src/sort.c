#include "internal.h"

#define MARGIN 5.f

int compare_rects_asc(const void* a, const void* b)
{
    SvgRect* ra = (SvgRect*)a;
    SvgRect* rb = (SvgRect*)b;

    float yDiff = ra->pos_y - rb->pos_y;
    if (fabsf(yDiff) > MARGIN)
    {
        return (int)yDiff;
    }

    float xDiff = ra->pos_x - rb->pos_x;
    if (fabsf(xDiff) > MARGIN)
    {
        return (int)xDiff;
    }

    return 0;
}

void sort_rects(SvgContent svg)
{
    qsort(svg.rects, svg.rect_count, sizeof(SvgRect), compare_rects_asc);
}

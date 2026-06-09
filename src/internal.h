#ifndef SVGTOGPL_MODULE
#define SVGTOGPL_MODULE

#include <alloc/module.h>
#include <measure/module.h>
#include <string/macros.h>
#include <string/module.h>

#define MATCHER_UNSET -1
#define MAX_MATCHERS 4
#define ERR_NO -1

#ifndef NULL_COLOR
#define NULL_COLOR (rgb){1, 2, 3}
#endif

#ifndef LOG_TRACE
#define LOG_TRACE false
#endif

#define TRACE(msg, ...)                                                        \
    if (LOG_TRACE) str_printc(msg, __VA_ARGS__);

StringMemory Strings;

typedef struct
{
    int count;
    // NOTE: We need to set the value pointer to after the struct
    //-> since the struct only stores the pointer
    //- Using the values[] or values[1] syntax would change this
    //-> But then we need to remember to NOT COPY THE STRUCT,
    // which is equally akward, because it might lead to errors down stream
    // -> So i rather remember to set the pointer once
    str* values;
} Args;

typedef struct
{
    uint8_t r, g, b;
} rgb;

typedef struct
{
    str content;
    str style;
    str fill_color_hex;
    float pos_x, pos_y;
    rgb color;

} SvgRect;

typedef struct
{
    int id;
    str content;
    str fill_color_hex;
    rgb color;
} SvgLinGradStop;

typedef struct
{
    int svg_bytes;
    char* file;
    char* cursor;
    SvgLinGradStop* stops;
    int stop_count;
    SvgRect* rects;
    int rect_count;

} SvgContent;

typedef struct
{
    str match;
    bool inclusive;
    int index;
} Matcher;

typedef struct
{
    bool active;
    float min_x;
    float min_y;

} RectFilter;

typedef struct
{
    /* found start and is currently matching */
    bool is_matching;
    Matcher start;
    Matcher end;
} SectionMatcher;

typedef struct
{
    str section;
    int matcher_idx;
} SectionizerResult;

typedef struct
{
    str content;
    const char* content_cursor;

    /* -1 when not set */
    int cur_matcher_idx;
    int matcher_count;

    // PERF: after adding the multi section checking,
    // even with just one section in it,
    //  the program runtime increase by 100 ms (260 -> 360)
    //  -> Would be interesting to figure out why at some point
    SectionMatcher matchers[MAX_MATCHERS];

} Sectionizer;

#endif

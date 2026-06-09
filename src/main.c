
#include "parsing.c"
// TASKLIST:
//  ✔ load the svg file, via cmd arg
//  ✔ find all rects
//  ✔ print their colors + x,y to the console
//  ✔ parse the values
//  - save the results to file
//  ✔ additional filter for boundary box
//  ✔ find values of lineargradients (just match to swatch num?)

Args read_args(Arena* mem, int argc, char** argv)
{
    int arraySize = argc * sizeof(str);
    str* argValues = (str*)arena_use(mem, arraySize);

    Args args = {.count = argc, .values = argValues};
    for (int i = 0; i < argc; i++)
    {
        args.values[i] = str_alloc(argv[i]);
    }

    return args;
}

typedef enum
{
    SECTION_RECT,
    SECTION_STOP
} SectionMatcherIds;

typedef enum
{
    STOPP_COLOR,
    STOPP_ID,
    STOPP_count
} StopPropIds;

typedef enum
{
    PROP_FILL,
    PROP_POS_X,
    PROP_POS_Y,
    PROP_count
} PropertyMatcherIds;

void run(str file, RectFilter filter, Arena* mem)
{
    Timer t = {};
    timer_start(&t);

    FILE* svg = fopen(file.chars, "rb");
    if (!svg)
    {
        str_printc("Unable to open file: '%'", STR(file));
        return;
    }

    SvgContent content = {};

    fseek(svg, 0, SEEK_END);
    content.svg_bytes = ftell(svg);
    fseek(svg, 0, SEEK_SET);

    content.file = (char*)arena_use(mem, content.svg_bytes);
    content.cursor = content.file;
    fread(content.file, 1, content.svg_bytes, svg);

    float readTime = timer_ms_since_start(&t);
    timer_start(&t);

    Sectionizer sn = sectionizer_init((str){content.file, content.svg_bytes});
    sectionizer_add(&sn, (Matcher){str_alloc("<rect"), true},
                    (Matcher){str_alloc(">"), true});
    sectionizer_add(&sn, (Matcher){str_alloc("<stop"), true},
                    (Matcher){str_alloc(">"), true});

    SectionizerResult res = sectionizer_next(&sn);
    bool withinStopSection = true;
    while (res.section.len > 0)
    {
        SectionMatcherIds secId = (SectionMatcherIds)res.matcher_idx;

        switch (secId)
        {
        case SECTION_RECT:
        {
            if (!content.rects) content.rects = mem->memory + mem->cursor;
            if (withinStopSection) withinStopSection = false;
            SvgRect* r = (SvgRect*)arena_use(mem, sizeof(SvgRect));
            r->content = res.section;
            content.rect_count++;
        }
        break;
        case SECTION_STOP:
        {
            if (!content.stops) content.stops = mem->memory + mem->cursor;
            if (!withinStopSection)
            {
                str_printc("Unexpected: Found stop section after starting "
                           "rects, skipping: '%'",
                           res.section);
            }
            else
            {
                SvgLinGradStop* r =
                    (SvgLinGradStop*)arena_use(mem, sizeof(SvgLinGradStop));
                r->content = res.section;
                content.stop_count++;
            }
        }
        break;
        default:
            assert(false);
        };

        res = sectionizer_next(&sn);
    }

    // --------------  PARSE STOPS / GRADIENTS ----------

    Matcher stopStart = {str_alloc("stop-color:"), false};
    Matcher stopEnd = {str_alloc(";"), false};
    Matcher idStart = {str_alloc("id=\""), false};
    Matcher idEnd = {str_alloc("\""), false};
    for (int i = 0; i < content.stop_count; i++)
    {
        SvgLinGradStop* stop = &content.stops[i];
        Sectionizer sn = sectionizer_init(stop->content);

        sectionizer_add(&sn, stopStart, stopEnd);
        sectionizer_add(&sn, idStart, idEnd);

        for (int s = 0; s < STOPP_count; s++)
        {
            SectionizerResult res = sectionizer_next(&sn);
            switch ((StopPropIds)res.matcher_idx)
            {
            case STOPP_COLOR:
            {
                stop->fill_color_hex = res.section;
                stop->color = hex_to_rgb(stop->fill_color_hex, content);
            }
            break;
            case STOPP_ID:
            {
                stop->id = number_within_str(res.section);
            }
            break;
            default:
                assert(false);
            }
        }

        str_pool_reset(&Strings.print_buffer);
    }

    // --------------  PARSE RECTS ----------------------

    SectionMatcher fill = {};
    fill.start = (Matcher){.match = str_alloc("fill:"), .inclusive = false};
    fill.end = (Matcher){.match = str_alloc(";"), .inclusive = false};

    Matcher xStart = {str_alloc("x=\""), false};
    Matcher yStart = {str_alloc("y=\""), false};
    Matcher posEnd = {str_alloc("\""), false};

    for (int i = 0; i < content.rect_count; i++)
    {
        SvgRect* rect = &content.rects[i];
        Sectionizer sn = sectionizer_init(rect->content);
        sectionizer_add(&sn, fill.start, fill.end);
        sectionizer_add(&sn, xStart, posEnd);
        sectionizer_add(&sn, yStart, posEnd);

        for (int s = 0; s < PROP_count; s++)
        {
            SectionizerResult res = sectionizer_next(&sn);

            switch ((PropertyMatcherIds)res.matcher_idx)
            {
            case PROP_FILL:
            {
                rect->fill_color_hex = res.section;
                rect->color = hex_to_rgb(rect->fill_color_hex, content);

                // lookup color from the gradients (stop-color sections)
                if (!memcmp(&rect->color, &NULL_COLOR, sizeof(rgb)))
                {
                    SvgLinGradStop stop =
                        find_color_by_id(res.section, content);
                    rect->fill_color_hex = stop.fill_color_hex;
                    rect->color = stop.color;
                }
            }
            break;
            case PROP_POS_X:
            {
                rect->pos_x = atof(res.section.chars);
            }
            break;
            case PROP_POS_Y:
            {
                rect->pos_y = atof(res.section.chars);
            }
            break;
            default:
                assert(false);
            };
        }

        if (!filter.active ||
            (rect->pos_x >= filter.min_x && rect->pos_y >= filter.min_y))
        {
            str_printc("(%,%) % (%,%,%)", FLOAT(rect->pos_x, 1),
                       FLOAT(rect->pos_y, 1), STR(rect->fill_color_hex),
                       NUM(rect->color.r), NUM(rect->color.g),
                       NUM(rect->color.b));

            if (!memcmp(&rect->color, &NULL_COLOR, sizeof(rgb)))
            {
                TRACE("Undetermined color section: '%'", STR(rect->content));
            }
        }
        str_pool_reset(&Strings.print_buffer);
    }

    float parsingTime = timer_ms_since_start(&t);
    timer_start(&t);

    // parse filename
    // open file
    // write header
    // write values

    float writeTime = timer_ms_since_start(&t);
    str_printc("Reading file: %, len: % KB in % ms | % ms parsing", STR(file),
               NUM(content.svg_bytes / 1024), FLOAT(readTime, 3),
               FLOAT(parsingTime, 3));
    str_printc("Found % stops & % rects sections", NUM(content.stop_count),
               NUM(content.rect_count));
}

int main(int argc, char** argv)
{
    Timer t = {};
    timer_start(&t);

    str_init(&Strings, 256, 4096, 256, 256);

    Arena mainmem = {};
    arena_init(&mainmem, 32 * 1024 * 1024);

    Args args = read_args(&mainmem, argc, argv);
    if (args.count < 2)
    {
        str_printc("Usage: <file.svg> [<minx> <miny>]");
        return 1;
    }

    RectFilter filter = {};
    if (args.count >= 4)
    {
        filter.active = true;
        filter.min_x = atof(args.values[2].chars);
        filter.min_y = atof(args.values[3].chars);
    }

    run(args.values[1], filter, &mainmem);

    float runTime = timer_ms_since_start(&t);
    str_printc("| % ms | Execution time", FLOAT(runTime, 3));

    return 0;
}

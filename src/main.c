
#include "parsing.c"
// TASKLIST:
//  ✔ load the svg file, via cmd arg
//  ✔ find all rects
//  ✔ print their colors + x,y to the console
//  ✔ parse the values
//  - save the results to file
//  - additional filter for boundary box
//  ✔ find values of lineargradients (just match to swatch num?)

StringMemory Strings;

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
    PROP_FILL,
    PROP_POS_X,
    PROP_POS_Y,
    PROP_count
} PropertyMatcherIds;

void run(str file, Arena* mem)
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
    str_printc("Reading file: %, len: % B in % ms", STR(file),
               NUM(content.svg_bytes), FLOAT(readTime, 3));

    Sectionizer sn = sectionizer_init_one(
        (str){content.file, content.svg_bytes},
        (Matcher){.match = str_alloc("<rect"), .inclusive = true},
        (Matcher){.match = str_alloc(">"), .inclusive = true});

    content.rects = mem->memory + mem->cursor;
    SectionizerResult res = sectionizer_next(&sn);
    while (res.section.len > 0)
    {
        // TODO: add parser for stop thingies
        SvgRect* r = (SvgRect*)arena_use(mem, sizeof(SvgRect));
        r->content = res.section;
        content.rect_count++;
        res = sectionizer_next(&sn);
    }

    SectionMatcher fill = {};
    fill.start = (Matcher){.match = str_alloc("fill:"), .inclusive = false};
    fill.end = (Matcher){.match = str_alloc(";"), .inclusive = false};

    Matcher xStart = {str_alloc("x=\""), false};
    Matcher yStart = {str_alloc("y=\""), false};
    Matcher posEnd = {str_alloc("\""), false};

    for (int i = 0; i < content.rect_count; i++)
    {
        SvgRect rect = content.rects[i];
        Sectionizer sn = sectionizer_init(rect.content);
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
                rect.fill_color_hex = res.section;
                // TODO: lookup gradient colors
                rect.color = hex_to_rgb(rect.fill_color_hex);
            }
            break;
            case PROP_POS_X:
            {
                rect.pos_x = atof(res.section.chars);
            }
            break;
            case PROP_POS_Y:
            {
                rect.pos_y = atof(res.section.chars);
            }
            break;
            default:
                assert(false);
            };
        }

        if (rect.pos_x >= -500 && rect.pos_y >= -500)
        {
            str_printc("(%,%) % (%,%,%)", FLOAT(rect.pos_x, 1),
                       FLOAT(rect.pos_y, 1), STR(rect.fill_color_hex),
                       NUM(rect.color.r), NUM(rect.color.g), NUM(rect.color.b));

            if (!memcmp(&rect.color, &(rgb){}, sizeof(rgb)))
            {
                str_printc("Wired section: '%'", STR(rect.content));
            }
        }
        str_pool_reset(&Strings.print_buffer);
    }

    str_printc("Found % rect sections", NUM(content.rect_count));
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
        str_printc("Usage: <file.svg>");
        return 1;
    }

    run(args.values[1], &mainmem);

    float runTime = timer_ms_since_start(&t);
    str_printc("| % ms | Execution time", FLOAT(runTime, 3));

    return 0;
}

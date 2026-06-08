#include "internal.h"

void matcher_reset(Matcher* matcher)
{
    matcher->index = 0;
}

bool found_match(SectionMatcher* section, char c, bool start)
{
    Matcher* matcher = start ? &section->start : &section->end;
    bool canMatch = start || section->is_matching;

    if (canMatch && matcher->index < matcher->match.len)
    {
        bool charMatches = matcher->match.chars[matcher->index++] == c;

        if (charMatches && matcher->index == matcher->match.len)
        {
            section->is_matching = start;
            matcher_reset(matcher);
            return true;
        }

        if (!charMatches) matcher_reset(matcher);
    }

    return false;
}

Sectionizer sectionizer_init(str content)
{
    return (Sectionizer){.content = content,
                         .content_cursor = content.chars,
                         .cur_matcher_idx = MATCHER_UNSET};
}

void sectionizer_add(Sectionizer* sec, Matcher start, Matcher end)
{
    assert(sec->matcher_count < MAX_MATCHERS);

    sec->matchers[sec->matcher_count].start = start;
    sec->matchers[sec->matcher_count].end = end;
    sec->matcher_count++;
}

Sectionizer sectionizer_init_one(str content, Matcher start, Matcher end)
{
    Sectionizer sec = sectionizer_init(content);
    // this works, because we don't track any state yet
    // -> so just copy the stuff to the sectionizer struct
    sectionizer_add(&sec, start, end);

    return sec;
}

bool has_matched(SectionMatcher* matcher, char ch, Sectionizer* content,
                 SectionizerResult* res)
{
    if (found_match(matcher, ch, true))
    {
        int strStartAdj =
            matcher->start.inclusive ? -(matcher->start.match.len - 1) : +1;
        res->section.chars = content->content_cursor + strStartAdj;
        return true;
    }
    else if (found_match(matcher, ch, false))
    {
        int strEndAdjustment =
            matcher->end.inclusive ? 1 : -(matcher->end.match.len - 1);
        res->section.len =
            (content->content_cursor + strEndAdjustment) - res->section.chars;

        content->content_cursor++;
        res->matcher_idx = content->cur_matcher_idx;
        return true;
    }

    return false;
}

SectionizerResult sectionizer_next(Sectionizer* sec)
{
    SectionizerResult cur = {};

    while (sec->content_cursor < sec->content.chars + sec->content.len)
    {
        char ch = *sec->content_cursor;

        if (sec->cur_matcher_idx != MATCHER_UNSET)
        {
            // matcher set, means we found an end
            if (has_matched(&sec->matchers[sec->cur_matcher_idx], ch, sec,
                            &cur))
            {
                cur.matcher_idx = sec->cur_matcher_idx;
                sec->cur_matcher_idx = MATCHER_UNSET;
                break;
            }
        }
        else
        {
            // idx was unset -> means we found a start
            for (int i = 0; i < sec->matcher_count; i++)
            {
                if (has_matched(&sec->matchers[i], ch, sec, &cur))
                {
                    sec->cur_matcher_idx = i;
                    break;
                }
            }
        }
        sec->content_cursor++;
    }

    return cur;
}

rgb hex_to_rgb(str hexStr)
{
    const char* hex = hexStr.chars;
    rgb color = {};

    if (hexStr.len == 7)
    {
        if (hex[0] == '#') hex++;
    }
    else if (hexStr.len < 6)
    {
        str_printc("Invalid hex string: '%'", STR(hexStr));
        return color;
    }

    // Validate characters are hex digits
    for (int i = 0; i < 6; i++)
    {
        if (!isxdigit((unsigned char)hex[i]))
        {
            str_printc("Hex string contains non hex characters: '%'",
                       STR(hexStr));
            return color;
        }
    }

    // Parse components
    char r_str[3] = {hex[0], hex[1], '\0'};
    char g_str[3] = {hex[2], hex[3], '\0'};
    char b_str[3] = {hex[4], hex[5], '\0'};

    color.r = (uint8_t)strtoul(r_str, NULL, 16);
    color.g = (uint8_t)strtoul(g_str, NULL, 16);
    color.b = (uint8_t)strtoul(b_str, NULL, 16);

    return color;
}

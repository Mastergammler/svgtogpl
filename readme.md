# Inkscape SVG to GPL parser

*Simple parser that parses color squares created form a inkscape svg to gpl.    
The default Inkscape export reorders the colors and juggles up the pallete that way*

`svgtogpl <file.svg> [<minx> <miny>]`

**How it works**
- The tool will search for all `<rect >` secitons in the svg and read the `fill:` value
- It will also read the `<linearGradient><stop>` sections, since those contain mapped colors
- It will lookup the colors referenced by id's in those sections
- It will order the squares based on y, x value (if defined, float margin = 5)
- It will write the gpl file in the same folder as the input file

**Dependencies**
[My strings lib](https://github.com/Mastergammler/StringPoolExperiments)



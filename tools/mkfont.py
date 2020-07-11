#!/usr/bin/env python3

from enum import IntFlag
from PIL import Image, ImageDraw, ImageFont
from struct import Struct
import sys

MAGIC = 0x746e4675
VERSION = 1

header_struct = Struct('<IBBHH')
group_struct = Struct('<II')
group_entry_struct = Struct('<I')
glyph_struct = Struct('<hhHH') # glyph_t contains a bitmap_t


class Font:
    class HeaderFlag(IntFlag):
        monospace = 1

    def __init__(self, font_name, size):
        self.font = ImageFont.truetype(font_name, size)
        self.ascent, self.descent = self.font.getmetrics()

    def is_monospace(self):
        i_width, _ = self.font.getsize('I')
        w_width, _ = self.font.getsize('W')
        return i_width == w_width

    def get_glyph(self, c):
        (width, height), (x_offset, y_offset) = self.font.font.getsize(c)
        assert width <= 255
        assert height <= 255
        assert x_offset >= -128 and x_offset <= 127
        assert y_offset >= -128 and y_offset <= 127

        if height == 0:
            return glyph_struct.pack(x_offset, y_offset, width, height)

        glyph = Image.new('1', (width, height))
        draw = ImageDraw.Draw(glyph)
        draw.text((-x_offset, -y_offset), c, font=self.font, fill=1)
        return glyph_struct.pack(x_offset, y_offset, width, height) + \
                glyph.tobytes('raw')

    def build(self, ranges):
        flags = 0
        if self.is_monospace():
            flags |= HeaderFlag.monospace

        header = header_struct.pack(MAGIC, VERSION, flags, self.ascent,
                self.descent)

        groups_length = 0
        for range_ in ranges:
            size = group_struct.size + group_entry_struct.size * \
                    (range_.stop - range_.start + 1)
            groups_length += size
        groups_length += group_struct.size # sentinel

        groups = bytearray()
        glyphs = bytearray()
        for range_ in ranges:
            group_length = group_struct.size + \
                    group_entry_struct.size * (range_.stop - range_.start + 1)
            group = bytearray(group_length)
            group_struct.pack_into(group, 0, range_.start, range_.stop - 1)
            
            group_offset = group_struct.size
            for c in range_:
                glyph = self.get_glyph(chr(c))
                glyph_offset = header_struct.size + groups_length + len(glyphs)
                group_entry_struct.pack_into(group, group_offset, glyph_offset)
                group_offset += group_entry_struct.size
                glyphs.extend(glyph)

            groups.extend(group)
        groups.extend(group_struct.pack(1, 0)) # sentinel

        return header + groups + glyphs


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--range', action='append', nargs=2,
            type=lambda x: int(x, 0), metavar=('FIRST', 'LAST'))
    parser.add_argument('FONT')
    parser.add_argument('SIZE', type=int)
    parser.add_argument('OUT')
    args = parser.parse_args()

    if args.range == None:
        print('at least one range is required', file=sys.stderr)
        sys.exit(1)

    ranges = []
    for first, last in args.range:
        ranges.append(range(first, last + 1))

    with open(args.OUT, 'wb') as f:
        font = Font(args.FONT, args.SIZE)
        f.write(font.build(ranges))    

import struct, zlib

def parse_font(path, chars):
    with open(path, 'rb') as f:
        data = f.read()
    ihdr_pos = data.find(b'IHDR')
    width, height = struct.unpack('>II', data[ihdr_pos+4:ihdr_pos+12])
    idat = b''
    pos = 0
    while True:
        pos = data.find(b'IDAT', pos)
        if pos == -1: break
        length = struct.unpack('>I', data[pos-4:pos])[0]
        idat += data[pos+4:pos+4+length]
        pos += 4 + length
    raw = zlib.decompress(idat)
    stride = 1 + width * 4

    def get_pixel(x, y):
        idx = y * stride + 1 + x * 4
        return (raw[idx], raw[idx+1], raw[idx+2], raw[idx+3])

    def is_color(x, y):
        p = get_pixel(x, y)
        return p[3] == 0

    pos = 0
    for p in range(width):
        col_empty = True
        for j in range(height):
            if not is_color(p, j):
                col_empty = False
                break
        if not col_empty:
            pos = p
            break
    space_w = pos
    print(f'Font {path}: width={width}, height={height}, space_w={space_w}')

    glyphs = {}
    for i in range(len(chars)):
        ch = chars[i]
        if ch == ' ':
            continue
        separator = pos
        while True:
            separator += 1
            if separator >= width:
                break
            found = True
            for j in range(height):
                if not is_color(separator, j):
                    found = False
                    break
            if found:
                break
        glyphs[ch] = (pos, separator - pos)
        pos = separator + 1

    return space_w, glyphs, height

LATIN_FULL = ' !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7f'

sw1, g1, h1 = parse_font('../assets/font1x.png', LATIN_FULL)
print(f'Parsed font1x glyphs: {len(g1)}')
sw2, g2, h2 = parse_font('../assets/font2x.png', LATIN_FULL)
print(f'Parsed font2x glyphs: {len(g2)}')

print('\nTesting 20/20 in font1x:')
for c in '20/20':
    print(f"'{c}': x={g1[c][0]}, w={g1[c][1]}")

print('\nTesting 20/20 in font2x:')
for c in '20/20':
    print(f"'{c}': x={g2[c][0]}, w={g2[c][1]}")

# Write C++ header/tables for exact accuracy
with open('gen_glyphs.inl', 'w') as out:
    out.write('// Auto-generated exact Noosa glyph tables\n')
    out.write('static constexpr struct { int16_t x; int16_t w; } GLYPHS_1X[95] = {\n')
    for i in range(1, len(LATIN_FULL)):
        ch = LATIN_FULL[i]
        x, w = g1[ch]
        ch_disp = ch if ch != '\\' and ch != '\'' else f'\\{ch}'
        out.write(f'    {{{x}, {w}}}, // [{ord(ch)}] \'{ch_disp}\'\n')
    out.write('};\n\n')

    out.write('static constexpr struct { int16_t x; int16_t w; } GLYPHS_2X[95] = {\n')
    for i in range(1, len(LATIN_FULL)):
        ch = LATIN_FULL[i]
        x, w = g2[ch]
        ch_disp = ch if ch != '\\' and ch != '\'' else f'\\{ch}'
        out.write(f'    {{{x}, {w}}}, // [{ord(ch)}] \'{ch_disp}\'\n')
    out.write('};\n')


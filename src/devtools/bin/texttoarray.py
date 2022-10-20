from pathlib import Path
import sys
import argparse

p = argparse.ArgumentParser()
p.add_argument('--file', type=str, required=True)
p.add_argument('--out', type=str, required=True)
p.add_argument('--name', type=str, required=True)
a = p.parse_args()

input = Path(a.file)
if not input or not input.exists():
    exit('missing --file argument')

output = a.out
if not output:
    output = input.with_suffix('.h')

var_name = a.name
if not var_name:
    var_name = f's_{input.stem.replace("-", "_")}'

with open(output, 'wt') as o:
    o.write('#pragma once\n\n')
    o.write(f'static constexpr const unsigned char {var_name}[] = ')
    o.write('{\n\t')
    with open(input, 'rb') as i:
        c = 0
        for b in i.read():
            o.write('0x%02x,' % b)
            c += 1
            if c >= 16:
                c = 0
                o.write('\n\t')
        c += 1
        if c >= 16:
            c = 0
            o.write('\n\t')
        o.write('0x00\n};')
#!/usr/bin/env python
# encoding: utf-8

'''
@author: Milos Subotic <milos.subotic.sm@gmail.com>
@license: MIT

'''

###############################################################################

import os
import sys
import argparse

from PIL import Image

###############################################################################

class Sprite:
	def __init__(self, name, w, h, el_type, el_format):
		self.name = name
		self.w = w
		self.h = h
		self.el_type = el_type
		self.el_format = el_format
		self.p = []

def gen_c_name_from_file_name(file_name):
	n = os.path.basename(os.path.splitext(file_name)[0])
	n = n.replace(' ', '_')
	n = n.replace('.', '_')
	return n

def img_to_src(
	format,
	output,
	inputs,
	prepend_palette,
	verbose = False
):
	assert(output.endswith('.c'))
	output_c = output
	output_h = os.path.splitext(output)[0] + '.h'
	output_h_def = gen_c_name_from_file_name(output_h).upper() + '_H'

	imgs = []
	img_names = []
	for i in inputs:
		imgs.append(Image.open(i))
		img_names.append(gen_c_name_from_file_name(i))
	
	
	sprites = []
	palette = []
	def print_palette_to(io):
		io.write('Palette:\n')
		for i in range(0, len(palette)-1):
			io.write('0x{:06x},\n'.format(palette[i]))
		io.write('0x{:06x}\n'.format(palette[-1]))
		
	
	if format == 'RGB333':
		for (img, img_name) in zip(imgs, img_names):
			w, h = img.size
			rgb_img = img.convert('RGB')
		
			sprite = Sprite(img_name, w, h, 'uint16_t', '0x{:03x}')
			sprites.append(sprite)
			
			for r in range(0, h):
				for c in range(0, w):
					R, G, B = rgb_img.getpixel((c, r))
					p = (B>>3 & 0x7)<<6 | (G>>3 & 0x7)<<3 | R>>5
					sprite.p.append(p)
		
	elif format == 'IDX4' or format == 'IDX1':
		if format == 'IDX4':
			b_per_c = 4
		elif format == 'IDX1':
			b_per_c = 1
		c_per_w = 32/b_per_c
		max_colors = 2**b_per_c
		
		if prepend_palette:
			l = prepend_palette.split(',')
			prepend_palette = [int(s, 16) for s in l]
		else:
			prepend_palette = []
		
		colors_to_count = {}
		for (img, img_name) in zip(imgs, img_names):
			w, h = img.size
		
			sprite = Sprite(img_name, w, h, 'uint32_t', '0x{:08x}')
			sprites.append(sprite)
			sprite.rgb_img = img.convert('RGB')
			
			for r in range(0, h):
				for c in range(0, w):
					R, G, B = sprite.rgb_img.getpixel((c, r))
					RGB = B<<16 | G<<8 | R
					if RGB not in colors_to_count:
						colors_to_count[RGB] = 0
					else:
						colors_to_count[RGB] += 1
		
		colors_and_counts = list(colors_to_count.items())
		def sort_fun(cc):
			return cc[1] # Sort by count.
		colors_and_counts.sort(key = sort_fun, reverse = True)
		
		color_to_idx = {}
		idx = 0
		for color in prepend_palette:
			color_to_idx[color] = idx
			idx += 1
			palette.append(color)
		for (color, count) in colors_and_counts:
			if color not in palette:
				color_to_idx[color] = idx
				idx += 1
				palette.append(color)
		
		if len(palette) > max_colors:
			sys.stderr.write(
				'Too many colors for palette: {} of {} possible!\n'.format(
					len(palette),
					max_colors
				)
			)
			print_palette_to(sys.stderr)
			sys.exit(1)
		else:
			for i in range(len(palette), max_colors):
				palette.append(0)
		
		for sprite in sprites:
			for r in range(0, sprite.h):
				cw = 0
				w = 0
				for c in range(0, sprite.w):
					R, G, B = sprite.rgb_img.getpixel((c, r))
					RGB = B<<16 | G<<8 | R
					IDX = color_to_idx[RGB]
					w |= IDX << cw*b_per_c
					cw += 1
					if cw == c_per_w:
						sprite.p.append(w)
						cw = 0
						w = 0
				if cw != 0:
					sprite.p.append(w)
		
		if verbose:	
			print_palette_to(sys.stdout)
	
	with open(output_c, 'w') as c:
		with open(output_h, 'w') as h:
			h.write('#ifndef {}\n'.format(output_h_def))
			h.write('#define {}\n'.format(output_h_def))
			h.write('\n')
			h.write('#include <stdint.h>\n')
			
			hn = os.path.basename(output_h)
			c.write('#include "{}"\n'.format(hn))
						
			if palette:
				h.write('\n')
				h.write('extern uint32_t palette[{}];\n'.format(len(palette)))
			
				c.write('\n')
				c.write('uint32_t palette[{}] = {{\n'.format(len(palette)))
				for i in range(0, len(palette)-1):
					c.write('\t0x{:06x},\n'.format(palette[i]))
				c.write('\t0x{:06x}\n'.format(palette[-1]))
				c.write('};\n')
			
			
			for sprite in sprites:
				n = sprite.name
				h.write('\n')
				h.write('extern uint16_t {}__w;\n'.format(n))
				h.write('extern uint16_t {}__h;\n'.format(n))
				h.write('extern {} {}__p[];\n'.format(sprite.el_type, n))
				
				c.write('\n')
				c.write('uint16_t {}__w = {};\n'.format(n, sprite.w))
				c.write('uint16_t {}__h = {};\n'.format(n, sprite.h))
				c.write('{} {}__p[] = {{\n'.format(sprite.el_type, n))
				i = 0
				while True:
					c.write('\t')
					for col in range(8):
						c.write(sprite.el_format.format(sprite.p[i]))
						i += 1
						if i == len(sprite.p):
							break
						if col < 7:
							c.write(', ')
					if i == len(sprite.p):
						c.write('\n')
						break
					else:
						c.write(',\n')
				c.write('}};\n'.format(sprite.el_type, n))
			
			h.write('\n')
			h.write('#endif // {}\n'.format(output_h_def))
			
	

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument(
		'-v',
		'--verbose',
		action = 'store_true',
		help = 'Print: palette'
	)
	parser.add_argument(
		'-f',
		'--format',
		required = True,
		choices=['IDX1', 'IDX4', 'RGB333'],
		help = 'Output format'
	)
	parser.add_argument(
		'-p',
		'--prepend-palette',
		required = False,
		help = 'Prepend palette: list of , separated RGB888 values'
	)
	parser.add_argument(
		'-o',
		'--output',
		required = True,
		help = 'output C/C++ file (Header will be create too)'
	)
	parser.add_argument(
		'inputs',
		nargs = '+',
		help = 'Input image files'
	)
	args = parser.parse_args()
	
	img_to_src(
		args.format,
		args.output,
		args.inputs,
		args.prepend_palette,
		args.verbose
	)

###############################################################################

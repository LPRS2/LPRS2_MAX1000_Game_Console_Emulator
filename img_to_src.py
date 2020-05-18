#! /usr/bin/env python
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
	inputs
):
	assert(output.endswith('.c'))
	output_c = output
	output_h = os.path.splitext(output)[0] + '.h'
	output_h_def = gen_c_name_from_file_name(output_h).upper() + '_H'

	imgs = []
	img_names = []
	for i in inputs:
		imgs.append(Image.open(i))
		img_names.append(gen_c_name_from_file_name(output))
	
	
	if format == 'RGB333':
		sprites = []
		for (img, img_name) in zip(imgs, img_names):
			w, h = img.size
			rgb_img = img.convert('RGB')
		
			sprite = Sprite(img_name, w, h, 'uint16_t', '0x{:03x}')
			
			for r in range(0, h):
				for c in range(0, w):
					R, G, B = rgb_img.getpixel((c, r))
					p = (B>>3 & 0x7)<<6 | (G>>3 & 0x7)<<3 | R>>5
					sprite.p.append(p)
		
		sprites.append(sprite)
		
	elif format == 'IDX4':
		el_type = 'uint32_t' # Because packing.
		#TODO
	
	with open(output_c, 'w') as c:
		with open(output_h, 'w') as h:
			h.write('#ifndef {}\n'.format(output_h_def))
			h.write('#define {}\n'.format(output_h_def))
			h.write('\n')
			h.write('#include <stdint.h>\n')
			h.write('\n')
		
			hn = os.path.basename(output_h)
			c.write('#include "{}"\n'.format(hn))
			
			for sprite in sprites:
				n = sprite.name
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
		'-f',
		'--format',
		required = True,
		choices=['IDX1', 'IDX4', 'RGB333'],
		help = 'Output format'
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
		args.inputs
	)

###############################################################################

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
	def __init__(self, name, w, h, el_type):
		self.name = name
		self.w = w
		self.h = h
		self.el_type = el_type
		self.array = []

def img_to_src(
	format,
	output,
	inputs
):
	assert(output.endswith('.c'))
	output_c = output
	output_h = os.path.splitext(output)[0] + '.h'

	imgs = []
	img_names = []
	for i in inputs:
		imgs.append(Image.open(i))
		n = os.path.base(os.path.splitext(output)[0])
		n = n.replace(' ', '_')
		n = n.replace('.', '_')
		img_names.append(n)
	
	
	if format == 'RGB333':
		sprites = []
		for (img, img_name) in zip(imgs, img_names):
			w, h = img.size
			rgb_img = img.convert('RGB')
		
			sprite = Sprite(img_name, w, h, 'uint16_t')
			
			for r in range(1, h):
				for c in range(1, w):
					R, G, B = rgb_img.getpixel((c, r))
					p = (B>>3 & 0x7)<<6 | (G>>3 & 0x7)<<3 | R>>5
					sprite.array.append(p)
		
		sprites.append(sprite)
		
	elif format == 'IDX4':
		el_type = 'uint32_t' # Because packing.
		#TODO
	
	with open(output_c, 'w') as cf:
		with open(output_h, 'w') as ch:
			for sprite in sprites:
				n = sprite.name
				
	

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

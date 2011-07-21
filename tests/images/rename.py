#!/usr/bin/python
import os

path = '.'
listing = os.listdir(path)

i = 1

for infile in listing:
	basename, ext = os.path.splitext(infile)
	if ext == ".bmp":
		os.rename(infile,"%03d.bmp"%(i))
		i = i + 1

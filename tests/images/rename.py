#!/usr/bin/python
import os
import os.path
import hashlib

def md5Checksum(filePath):
    fh = open(filePath, 'rb')
    m = hashlib.md5()
    while True:
        data = fh.read(8192)
        if not data:
            break
        m.update(data)
    return m.hexdigest()

path = '.'
listing = os.listdir(path)

for infile in listing:
	basename, ext = os.path.splitext(infile)
	if ext == ".bmp":
		os.rename(infile,md5Checksum(infile) + ".bmp")

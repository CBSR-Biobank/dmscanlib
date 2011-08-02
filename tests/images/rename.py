#!/usr/bin/python
import os
import os.path
import sys
import hashlib


path = '.'
listing = os.listdir(path)

def md5Checksum(filePath):
    fh = open(filePath, 'rb')
    m = hashlib.md5()
    while True:
        data = fh.read(8192)
        if not data:
            break
        m.update(data)
    return m.hexdigest()

def main(argv):
	
	if len(argv) != 1 or argv[0] != "rename":
		print "Renames all the bmp files by there md5 hash in this scripts working directory."
		print "Usuage thiscript.py rename."
		return

	print "Renaming files...\n"
	for infile in listing:
		basename, ext = os.path.splitext(infile)
		if ext == ".bmp":
			md5check = md5Checksum(infile) +".bmp"
			print "renaming %s to %s" % (infile,md5check)
			os.rename(infile, md5check)
	
	print "Renamed files.\n"

if __name__ == "__main__":
	main(sys.argv[1:])

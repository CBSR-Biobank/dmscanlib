#!/usr/bin/python

import sys,getopt,os,datetime,re, time
import subprocess, threading, signal,random

import padnums

try: import simplejson as json
except ImportError: import json

scanlibDirectory = "../Linux-Debug"
scanlibExecutable = "dmscanlib"
scanLibBarcodesFileName = "dmscanlib.txt"
scanLibTimeout = 30


class Barcode(object):
	position = None
	value = ""

	def __init__(self,message=None,position=None,value=None):

		if position != None and value != None:
			self.position = position
			self.value = value
			return

		if message == None:
			return None

		matches = re.match(r"\d,([A-H]),(\d{1,2}),([A-Z0-9]+)",message.strip())

		if matches == None or matches.group(1) == None or \
		   matches.group(2) == None or matches.group(3) == None:
			return None

		row = matches.group(1)
		col = matches.group(2)
		barcodeStr =  matches.group(3)

		if barcodeStr == None or len(row) != 1 or len(col) < 1 or len(col) > 2:
			return None

                print row,col,barcodeStr

		self.position = [0,0]
		self.position[0] = ord(row) - ord('A')
		self.position[1] = int(col) - 1

		self.value = barcodeStr


	def getValue(self):
		return self.value

	def getPosition(self):
		return self.position

	def __str__(self):
		return "row/%02d col/%02d barcode/%s" % (int(self.position[0]),int(self.position[1]),self.value)

	def getDict(self):
		return {'position': self.position, 'value': self.value}

class ScanResult(object):
	timeTaken = 0
	barcodes = []
	filename = ""

	def __init__(self,timeTaken,barcodes,filename):
		self.timeTaken = timeTaken
		self.barcodes = barcodes
		self.filename = filename

	def getTimeTaken(self):
		return self.timeTaken

	def getBarcodesCount(self):
		return len(self.barcodes)

	def getBarcodes(self):
		return self.barcodes

	def getFilename(self):
		return self.filename

	def getBarcodesDictList(self):
		return map(lambda x: x.getDict(), self.getBarcodes())

	def __str__(self):
		return "\ntimeTaken: %d\nfilename: %s\n" % (self.timeTaken,self.filename) + "\n".join(map(lambda x: str(x), self.getBarcodes()))


class ScanResultContainer(object):
	scanResults = None

	def __init__(self,scanResults):
		 self.scanResults = scanResults

	def getScanResultFromFilename(self,filename):
		for sc in self.scanResults:
			if sc.getFilename() == filename:
				return sc

	def getResults(self):
		return self.scanResults

	def getDict(self):
		return map(lambda x:{'filename': x.getFilename(),'timeTaken': x.getTimeTaken(), 'barcodes': x.getBarcodesDictList() },self.scanResults)

	def __str__(self):
		return "\n".join(map(lambda x: str(x), self.scanResults))

	def save(self,filename):
		data = self.getDict()
		encoded = json.dumps(data)
		outputFileHandler = open(filename, 'wb')
		outputFileHandler.write(encoded)

	def load(self,filename):
		inputFileHandler = open(filename, 'rb')
		encoded = inputFileHandler.read()
		inputFileHandler.close()
		data =  json.loads(encoded)

		self.scanResults = []
		for scanresult in data:
			barcodeList = []
			for barcode in scanresult['barcodes']:
				barcodeList.append(Barcode(position=barcode['position'],value=barcode['value']))

			self.scanResults.append(ScanResult(scanresult['timeTaken'], barcodeList,scanresult['filename']))

def usage():
	print """
	There are 3 main modes of operation. "Test", "Show" and "Compare".
	Please select one of the modes and then specify the required arguments.

	This test program works in 2 stages. First you run the "Test" mode on a set of images.
	This will create a results file that contains the time taken and barcodes scanned for each image with
	the currently checked version of scanlib. Then you run the "Compare" stage, this mode is for camparing the result
	files from the "Test" mode.
	Usuage:
		./thisscript.py (-c|--compare) --results1=RESULTS_FILE_A --results2=RESULTS_FILE_B
		./thisscript.py (-s|--show) --results1=RESULTS_FILE_A
		./thisscript.py (-t|--test) (-i IMG_DIR|--imageDir=IMG_DIR) (-o RESULTS_FILE|--output=RESULTS_FILE)

	Examples:
		./thisscript.py --test --imageDir=images -o results.txt
		./thisscript.py -c --results1=scanlib_resultsApril.txt --results2=scanlib_resultsJune.txt
	"""

def exitProgramBad():
	print ""
	sys.exit(2);

def seperator(title = ""):
	print "\n==================%s==================" % title

class Command(object):

	def __init__(self, cmd):

		self.cmd = cmd
		self.process = None
		self.output = None

	def run(self, timeout):

		def target():
			self.process = subprocess.Popen(self.cmd, shell=False,stderr=subprocess.STDOUT,stdout=subprocess.PIPE)
		        self.output = self.process.communicate()

		thread = threading.Thread(target=target)
		thread.start()
		thread.join(timeout)

		if thread.is_alive():
			print "Terminating Process: %s" % (" ").join(self.cmd)
			self.process.terminate()
			thread.join()
			return ""


		if self.output[0] == None: return ""
		return self.output[0]

def runScanlib(scanLibPath,imagePath):
	timeStart = datetime.datetime.now()

	command = Command([scanLibPath,"--outputBarcodes", "-d" ,"--plate", "1" ,"-i" ,imagePath])
	scanlibOutput = command.run(timeout=scanLibTimeout)

	if scanlibOutput == None: return None

	barcodesMatch = re.findall(r"\d,[A-H],\d{1,2},[A-Z0-9]+",scanlibOutput)# scanlibPipe.read()
	if(len(barcodesMatch)  <= 0): return None

	barcodes = map(Barcode,barcodesMatch)

	#print "\n".join(map(lambda x: "%d,%d,%d,%d"%(x.position[0],x.position[1],x.position[2],x.position[3]),barcodes)) # barcodes values printeed
	#print "\n".join(map(lambda x: str(x),barcodes)) # barcodes values printeed

	timeEnd = datetime.datetime.now()

	timeDiff = timeEnd - timeStart
	timeDelta = timeDiff.seconds*1000 + timeDiff.microseconds/1000

	return ScanResult(timeDelta,barcodes,os.path.basename(imagePath))

def generalTest(imageDir,outputFile):
	seperator("General Test")

	print "Working directory: %s\n" % os.getcwd()

	imageDirFiles = ""

	try:
		imageDirFiles = os.listdir(imageDir)
	except os.error as detail:
		print 'Handling run-time error:%s'% detail
		exitProgramBad()

	imageDirFiles = filter(lambda x: x[-4:] ==".bmp",imageDirFiles)

	print "Processing %d images..." % len(imageDirFiles)

	scanResults = []

	for imagePath in imageDirFiles:
		print "Processing: %s" % imagePath

		if not os.path.isfile(imageDir + "/" + imagePath):
			print "Error: could not find %s in the image directory." % imagePath
			exitProgramBad()

		sc = runScanlib(scanlibDirectory + "/" + scanlibExecutable,imageDir +"/" + imagePath);
		if not sc == None:
			scanResults.append(sc)


	sr = ScanResultContainer(scanResults)

	print "Writing results to: %s" % outputFile
	sr.save(outputFile)
	print "Wrote to file."

def compareTest(resultsFile1,resultsFile2):
	seperator("Comparison Test")

	if not os.path.isfile(resultsFile1):
		print "Could not load results file 1"
		exitProgramBad()

	if not os.path.isfile(resultsFile2):
		print "Could not load results file 2"
		exitProgramBad()

	src1 = ScanResultContainer("")
	src2 = ScanResultContainer("")

	src1.load(resultsFile1)
	src2.load(resultsFile2)

	filenames1 = map(lambda x: os.path.basename(x.getFilename()),src1.scanResults)
	filenames2 =  map(lambda x: os.path.basename(x.getFilename()),src2.scanResults)

	table = [["","Image", "Barcodes Scanned", "Time Duration"]]

	total_bc_1, total_bc_2 = [0,0]
	total_t_1, total_t_2 = [0,0]

	for sf in [val for val in filenames1 if val in filenames2]: #intersection
		sr1 = src1.getScanResultFromFilename(sf)
		sr2 = src2.getScanResultFromFilename(sf)

		bc1 = sr1.getBarcodesCount()
		bc2 = sr2.getBarcodesCount()

		total_bc_1 += bc1
		total_bc_2 += bc2

		pbc = 0
		try: pbc = (bc2 / (bc1*1.00) - 1)*100
		except ZeroDivisionError: pass

		t1 = sr1.getTimeTaken()
		t2 = sr2.getTimeTaken()

		total_t_1 += t1
		total_t_2 += t2

		pt = 0
		try: pt = (t2 / (t1*1.00) -1 )*100
		except ZeroDivisionError: pass


		table.append(["",sf,"%03.1f%%(%d/%d)"%(pbc,bc2,bc1),"%03.1f%%(%d/%d)"%(pt,t2,t1)])




	ptotal_bc = 0
	try: ptotal_bc = (total_bc_2  / (total_bc_1*1.00) - 1)*100.0
	except ZeroDivisionError: pass

	ptotal_t = 0
	try: ptotal_t = (total_t_2  / (total_t_1*1.00) - 1)*100.0
	except ZeroDivisionError: pass

	table.append(["","","",""])
	table.append(["total:","","%03.2f%%(%d/%d)" %(ptotal_bc,total_bc_2,total_bc_1),
				  "%03.2f%%(%d/%d)" %(ptotal_t,total_t_2,total_t_1)])

	padnums.pprint_table(sys.stdout, table)
	print ""

def main(argv):

	try:
		opts, args = getopt.getopt(argv, "scthi:o:", ["show","compare","test","help", "imageDir=","output=","results1=","results2="])
	except getopt.GetoptError:
		usage()
		exitProgramBad()

	if not os.path.isfile(scanlibDirectory + "/" + scanlibExecutable):
		print "\nCould not find the scanlib executable at the predefined location: '%s'" % scanlibExecutable
		exitProgramBad()

	compare,test,show = [False,False,False]
	imageDir,resultsFileOutput,resultsFile1, resultsFile2 = ["","","",""]

	for opt, arg in opts:
		if opt in ("-h", "--help"):
			usage()
			sys.exit()
		elif opt in ("-s", "--show"):
			show = True
		elif opt in ("-c", "--compare"):
			compare = True
		elif opt in ("-t", "--test"):
			test = True
		elif opt in ("-i", "--imageDir"):
			imageDir = arg
		elif opt in ("-o", "--output"):
			resultsFileOutput = arg
		elif opt in ( "--results1"):
			resultsFile1 = arg
		elif opt in ( "--results2"):
			resultsFile2 = arg

	seperator("Arguments")
	print "show: %r" % show
	print "compare: %r" % compare
	print "test: %r" % test
	print "imageDir: " + imageDir
	print "resultsFileOutput: " + resultsFileOutput
	print "resultsFile1: " + resultsFile1
	print "resultsFile2: " + resultsFile2


	if sum(map(lambda x: int(x),[test,compare,show])) != 1:
		print "Invalid Options specified. Mutliple mode were selected."
		usage()
		exitProgramBad()

	if test and imageDir != "" and resultsFileOutput != "":
		seperator("Test Case")
		print "Peforming test case: General Test"
		generalTest(imageDir,resultsFileOutput)

	elif compare and resultsFile1 != "" and resultsFile2 != "":
		seperator("Test Case")
		print "Peforming test case: Comparison Test"
		compareTest(resultsFile1,resultsFile2)

	elif show and (resultsFile1 != ""):
		if os.path.isfile(resultsFile1):
			seperator("Results File 1")
			src = ScanResultContainer("")
			src.load(resultsFile1)
			print src

	else:
		print "Invalid Options specified."
		usage()
		exitProgramBad()

if __name__ == "__main__":
	main(sys.argv[1:])
	print "Main finished"
	sys.exit()

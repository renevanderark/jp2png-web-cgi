#!/usr/bin/python3

import threading
import urllib
import urllib.request
import urllib.parse
import time
import sys
import os

class Task(urllib.request.HTTPHandler,urllib.request.HTTPErrorProcessor):
	def setup(self, url, saveOutput):
		self.before = int(round(time.time() * 1000))
		self.url = url
		self.saveOutput = saveOutput

	def http_response(self, req, response):
		self.timed = int(round(time.time() * 1000)) - self.before
		if(self.saveOutput != False):
			data = response.read()
			f = open("./out/" + self.saveOutput, "wb")
			f.write(data)
			f.close()

	def report(self):
		print(str(self.url) + ";" + str(self.timed))


num_tiles = 130
num_pages = 34
num_papers = 10
max_concurrent = 50
res = 1
page = 0
paper = 0
openThreads = []
executedTasks = []
saveOutput = False
if(len(sys.argv) > 1 and sys.argv[1] == "--save-response"):
	saveOutput = True
	if(os.path.isdir("./out") == False):
		os.mkdir("./out")
	print("Saving responses to dir ./out")

print("Executing parallel full image processing at resolution level 5")
while paper < num_papers:
	image_url = "http://resolver.kb.nl/resolve?urn=ddd:0" + str(10691742 + paper) + ":mpeg21:p001:image"
	viewer_url = "http://localhost/cgi-bin/jp2?i=1&u=" + urllib.parse.quote(image_url) + "&r=5"
	task = Task()
	outfile = False
	if(saveOutput):
		outfile = "thumb-" + str(page) + ".png"
	task.setup(viewer_url, outfile)
	opener = urllib.request.build_opener(task)
	urllib.request.install_opener(opener)
	if(len(openThreads) >= max_concurrent):
		t = openThreads.pop(0)
		t.join()
		sys.stderr.write("Joined: " + t.getName() + " / open: " + str(len(openThreads)) + " of total: " + str(num_papers) + "\n")
		sys.stdout.write(".")
		sys.stdout.flush()


	t = threading.Thread(target=opener.open, args=(viewer_url,))
	openThreads.append(t)
	executedTasks.append(task)
	t.start()
	paper += 1

for i in range(len(openThreads)):
	t = openThreads.pop(0)
	t.join()
	sys.stderr.write("Joined: " + t.getName() + " / open: " + str(len(openThreads)) + " of total: " + str(num_tiles * num_pages) + "\n")
	sys.stdout.write(".")
	sys.stdout.flush()


s = 0
for i in range(len(executedTasks)):
	s += executedTasks[i].timed

print("Average response time: " + str(s / len(executedTasks)) + "ms")

print("Executing parallel tile processing at resolution level " + str(res))
while page < num_pages:
	image_url = "http://resolver.kb.nl/resolve?urn=ddd:010691742:mpeg21:p" + str(page + 1).zfill(3) + ":image"
	tile = 0
	while tile < num_tiles:
		viewer_url = "http://localhost/cgi-bin/jp2?t=" + str(tile) + "&u=" + urllib.parse.quote(image_url) + "&r=" + str(res)
		task = Task()
		outfile = False
		if(saveOutput):
			outfile = str(page) + "-" + str(tile) + ".png"
		task.setup(viewer_url, outfile)
		opener = urllib.request.build_opener(task)
		urllib.request.install_opener(opener)
		if(len(openThreads) >= max_concurrent):
			t = openThreads.pop(0)
			t.join()
			sys.stderr.write("Joined: " + t.getName() + " / open: " + str(len(openThreads)) + " of total: " + str(num_tiles * num_pages) + "\n")
			sys.stdout.write(".")
			sys.stdout.flush()
		t = threading.Thread(target=opener.open, args=(viewer_url,))
		openThreads.append(t)
		executedTasks.append(task)
		t.start()
		tile += 1
	page += 1

for i in range(len(openThreads)):
	t = openThreads.pop(0)
	t.join()
	sys.stderr.write("Joined: " + t.getName() + " / open: " + str(len(openThreads)) + " of total: " + str(num_tiles * num_pages) + "\n")
	sys.stdout.write(".")
	sys.stdout.flush()

s = 0
for i in range(len(executedTasks)):
	s += executedTasks[i].timed

print("Average response time: " + str(s / len(executedTasks)) + "ms")

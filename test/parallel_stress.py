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
max_concurrent = 50
page = 0
openThreads = []
executedTasks = []
saveOutput = False
if(len(sys.argv) > 1 and sys.argv[1] == "--save-response"):
	saveOutput = True
	if(os.path.isdir("./out") == False):
		os.mkdir("./out")
	print("Saving responses to dir ./out")

while page < num_pages:
	image_url = "http://resolver.kb.nl/resolve?urn=ddd:010691742:mpeg21:p" + str(page + 1).zfill(3) + ":image"
	tile = 0
	while tile < num_tiles:
		viewer_url = "http://localhost/cgi-bin/jp2?t=" + str(tile) + "&u=" + urllib.parse.quote(image_url) + "&r=1"
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
			print("Joined: " + t.getName() + " / open: " + str(len(openThreads)) + " of total: " + str(num_tiles * num_pages))
		t = threading.Thread(target=opener.open, args=(viewer_url,))
		openThreads.append(t)
		executedTasks.append(task)
		t.start()
		tile += 1
	page += 1

for i in range(len(openThreads)):
	t = openThreads.pop(0)
	t.join()
	print("Joined: " + t.getName() + " / open: " + str(len(openThreads)) + " of total: " + str(num_tiles * num_pages))

s = 0
for i in range(len(executedTasks)):
	s += executedTasks[i].timed

print("Average response time: " + str(s / len(executedTasks)) + "ms")

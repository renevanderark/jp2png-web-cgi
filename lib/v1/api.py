#!/usr/bin/python
import urllib2
from PIL import Image
import os, io, time
import threading
                

class Task(urllib2.HTTPHandler,urllib2.HTTPErrorProcessor):
        def setup(self, x, y, composed_image, shared_res):
                self.before = int(round(time.time() * 1000))
                self.x = x
                self.y = y
                self.composed_image = composed_image
                self.shared_res = shared_res

        def http_response(self, req, response):
                jpeg = Image.open(io.BytesIO(response.read()))
                self.composed_image.paste(jpeg, (self.x,self.y))
                self.shared_res.increment()
                print int(round(time.time() * 1000)) - self.before

class SharedResource():
        def __init__(self):
                self.num = 0

        def increment(self):
                self.num += 1

def get_tiles(tiles, shared_res, composed_image, url):
        for i in range(len(tiles)):
                task = Task()
                y = int(tiles[i] / 10)
                x = tiles[i] - (y * 10)
                task.setup( x * 256, y * 256, composed_image, shared_res)
                opener = urllib2.build_opener(task)
                opener.open(url + str(tiles[i]))

max_threads = 12
num_tiles = 50

image_url = "http://resolver.kb.nl/resolve?urn=ddd:010691742:mpeg21:p001:image"
viewer_url = "http://localhost/cgi-bin/jp2?&u=" + urllib2.quote(image_url) + "&r=1&j=1&t="

composed_image = Image.new("RGB", (10*256,5*256))
shared_res = SharedResource()

distr = [[],[],[],[],[],[],[],[],[],[],[],[],[],[],[],[]]
for i in range(num_tiles):
        distr[i%max_threads].append(i)

for i in range(len(distr)):
        t = threading.Thread(target=get_tiles, args=(distr[i], shared_res, composed_image, viewer_url,))
        t.start()

while shared_res.num < num_tiles:
        pass

composed_image.save("test.jpg")

#task = Task()
#task.setup(20, 20, composed_image, shared_res)
#task.setup(40, 50, composed_image, shared_res)
#opener = urllib2.build_opener(task)
#opener = urllib2.build_opener(task)
#opener.open(viewer_url)

#print shared_res.num


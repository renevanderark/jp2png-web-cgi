#!/usr/bin/python3
import os
import subprocess
import unittest
import datetime
import json

class Jp2ReadHTTPTest(unittest.TestCase):
	def __init__(self, name, query_string, content_type, header_bytes=137):
		unittest.TestCase.__init__(self, name)
		os.putenv("QUERY_STRING", query_string)
		self.results = subprocess.Popen("../jp2.cgi", stdout=subprocess.PIPE).communicate()[0]
		self.content_type = content_type
		self.headers = self.results[:header_bytes].decode('utf-8').split('\n')

	def runHTTPHeaderTest(self):
		self.assertEqual(self.headers[0], 'Content-type: ' + self.content_type)
		self.assertEqual(self.headers[1], 'Pragma: public')
		self.assertEqual(self.headers[2], 'Cache-Control: max-age=360000')
		header = self.headers[3][:13]
		timestamp = self.headers[3][15:]
		self.assertEqual(header, 'Last-Modified')
		tm = datetime.datetime.strptime(timestamp, "%a, %d %b %Y %H:%M:%S GMT")
		today = datetime.date.today()
		self.assertEqual(today.day, tm.day, "Last modified header day should match today")
		self.assertEqual(today.month, tm.month, "Last modified header month should match this month")
		self.assertEqual(today.year, tm.year, "Last modified header year should match this year")
		self.assertEqual(self.headers[4], 'Status: 200 OK')

	def runJP2HeaderTest(self):
		jp2h = json.loads(self.results[138:].decode('utf-8'))
		self.assertEqual(jp2h['tw'], 3, "balloon.jp2 has 3x4 tiles (tw=3)")
		self.assertEqual(jp2h['th'], 4, "balloon.jp2 has 3x4 tiles (th=4)")
		self.assertEqual(jp2h['num_comps'], 3, "balloon.jp2 has 3 compositions (num_comps=3)")
		self.assertEqual(jp2h['tdx'], 1024, "balloon.jp2 has tiles of 1024x1024 pixels (tdx=1024)")
		self.assertEqual(jp2h['tdy'], 1024, "balloon.jp2 has tiles of 1024x1024 pixels (tdy=1024)")
		self.assertEqual(jp2h['x1'], 2717, "balloon.jp2 is 2717x3701 pixels in size (y1=2717)")
		self.assertEqual(jp2h['y1'], 3701, "balloon.jp2 is 2717x3701 pixels in size (x1=3701)")
		self.assertEqual(jp2h['num_res'], 6, "balloon.jp2 has 6 resolution levels (num_res=6)")

	def runPNGTest(self):
		bytes = self.results[131:]

if __name__ == '__main__':
	runner = unittest.TextTestRunner()
	test_suite = unittest.TestSuite()
	test_suite.addTest(Jp2ReadHTTPTest("runHTTPHeaderTest", query_string="f=../balloon.jp2", content_type="application/json"))
	test_suite.addTest(Jp2ReadHTTPTest("runHTTPHeaderTest", query_string="f=../balloon.jp2&t=1&r=5", content_type="image/png", header_bytes=130))
	test_suite.addTest(Jp2ReadHTTPTest("runJP2HeaderTest", query_string="f=../balloon.jp2", content_type="application/json"))
#	test_suite.addTest(Jp2decodeTileTest())
	runner.run (test_suite)

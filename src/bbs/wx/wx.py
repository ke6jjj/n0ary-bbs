#!/usr/bin/env python
import urllib2
import urllib
import json
import time
import math

data_base = 'http://meso-wx/data.php'
entity_id = 'weewx_raw'

bounded_items = [
  ('outTemp', 'f', 0),
  ('outHumidity', 'perc', 0),
  ('barometer', 'inHg', 2),
  ('windSpeed', 'mph', 0),
  ('windDir', 'deg', 0),
]

items = [
  'dayRain:max:in:2'
]

def bounded_item_str(item_desc):
  return [ '{}:{}:{}:{}'.format(item_desc[0], agg, item_desc[1], item_desc[2])
           for agg in ('min', 'avg', 'max') ]

def current_item_str(item_desc):
  return [ '{}::{}:{}'.format(item_desc[0], item_desc[1], item_desc[2] ) ]

def do_summary():
  item_strs = []
  for item in bounded_items:
    item_strs += bounded_item_str(item)
  item_strs += items

  now = int(time.time())
  now_sub_hour = now % 3600
  next_hour = now + 3600 - now_sub_hour
  day_ago = next_hour - 86400

  url = data_base + '?entity_id=' + entity_id + '&data=' + ','.join(item_strs) \
    + '&start={}:datetime&end={}:datetime'.format(day_ago, next_hour) \
    + '&group=3600:seconds'

  r = urllib2.urlopen(url)
  t = r.read()
  data = json.loads(t)

  print len(data)
  for item in data:
    # Adjust barometer to integer
    item[7] = int(item[7] * 100)
    item[8] = int(item[8] * 100)
    item[9] = int(item[9] * 100)
    # Adjust rain to integer
    item[16] = int(item[16] * 100)
  
    print ' '.join([str(x) for x in item])

def winddir(deg):
  quad = int((deg + 22.5) / 45.0)
  quad = quad % 8
  return ('N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW' )[quad]

def do_current():
  item_strs = [ 'dateTime::s' ]
  for item in bounded_items:
    item_strs += current_item_str(item)
  item_strs += items
  item_strs.append('rainRate::inHr')

  url = data_base + '?entity_id=' + entity_id + '&data=' + ','.join(item_strs) \
    + '&order=desc&limit=1'

  print url

  r = urllib2.urlopen(url)
  t = r.read()
  data = json.loads(t)
  assert(len(data) == 1)
  data = data[0]

  now = data[0]
  print "Weather Data as of {}".format(time.ctime(now))
  print "Bernal Heights, San Francisco El 87'"
  print "                   Current  Rate"
  print "Temperature (degF)  %3d" % data[1]
  print "Humidity (%%)        %3d" % data[2]
  print "Barometer (in)       %5.2f" % data[3]
  print "Rain (in)            %5.2f %5.2f" % (data[6], data[7])

  print ""
  print "Wind (mph)          %03d deg (%s) @ %3d" % (data[5], winddir(data[5]), data[4])

def main(args):
  if len(args) < 2:
    print >>sys.stderr, "usage: wx current|summary"
    return 1

  if args[1] == 'current':
    do_current()
  elif args[1] == 'summary':
    do_summary()
  else:
    print >>sys.stderr, "unknown verb"
    return 1

  return 0

if __name__ == '__main__':
  import sys

  sys.exit(main(sys.argv))

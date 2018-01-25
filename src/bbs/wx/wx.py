#!/usr/bin/env python
import urllib2
import urllib
import json
import time
import math
import collections

data_base = 'http://meso-wx/data.php'
entity_id = 'weewx_raw'

bounded_items = [
  ('outTemp', 'f', 0),
  ('outHumidity', 'perc', 0),
  ('barometer', 'inHg', 2),
  ('windSpeed', 'mph', 0),
  ('windDir', 'deg', 0),
  ('radiation', 'wsqm', 0),
  ('uv', 'uvi', 0),
]

items = [
  'dayRain:max:in:2'
]

def bounded_item_str(item_desc):
  return [ '{}:{}:{}:{}'.format(item_desc[0], agg, item_desc[1], item_desc[2])
           for agg in ('min', 'avg', 'max') ]

def bounded_item_name(item_desc):
  return [ '{}_{}'.format(item_desc[0], agg) for agg in ('min', 'avg', 'max') ]

def current_item_str(item_desc):
  return [ '{}::{}:{}'.format(item_desc[0], item_desc[1], item_desc[2] ) ]

def do_summary():
  item_strs = []
  names = ['when']
  for item in bounded_items:
    item_strs += bounded_item_str(item)
    names += bounded_item_name(item)
  item_strs += items
  names.append('dayRain')

  now = int(time.time())
  now_sub_hour = now % 3600
  next_hour = now + 3600 - now_sub_hour
  day_ago = next_hour - 86400

  wx_row = collections.namedtuple('wx_row', names)

  url = data_base + '?entity_id=' + entity_id + '&data=' + ','.join(item_strs) \
    + '&start={}:datetime&end={}:datetime'.format(day_ago, next_hour) \
    + '&group=3600:seconds'

  r = urllib2.urlopen(url)
  t = r.read()
  data = json.loads(t)

  print len(data)
  for row in data:
    data = wx_row(*row)
    # Adjust some items
    data = data._replace(
      # Adjust barometer to integer
      barometer_min=int(data.barometer_min * 100),
      barometer_avg=int(data.barometer_avg * 100),
      barometer_max=int(data.barometer_max * 100),
      # Adjust rain to integer
      dayRain=int(data.dayRain * 100),
    )
    print ' '.join(
      map(lambda x: str(x) if x is not None else '0', [
        data.when,
        data.outTemp_min,
        data.outTemp_avg,
        data.outTemp_max,
        data.outHumidity_min,
        data.outHumidity_min,
        data.outHumidity_min,
        data.barometer_avg,
        data.barometer_avg,
        data.barometer_avg,
        data.windSpeed_min,
        data.windDir_min,
        data.windSpeed_avg,
        data.windDir_avg,
        data.windSpeed_max,
        data.windDir_max,
        data.radiation_min,
        data.radiation_avg,
        data.radiation_max,
        data.uv_min,
        data.uv_avg,
        data.uv_max,
        data.dayRain
        ]
      )
    )
       
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

  r = urllib2.urlopen(url)
  t = r.read()
  data = json.loads(t)
  assert(len(data) == 1)
  # Unpack single row
  data = collections.namedtuple('Weather',
    ['when','temperature','humidity','pressure','windSpeed','windDir',
     'solarRad', 'uv', 'rainTotal','rainRate'])(*data[0])

  print "Weather Data as of {}".format(time.ctime(data.when))
  print "Bernal Heights, San Francisco El 87'"
  print ""
  print "Temperature       %d F" % data.temperature
  print "Humidity          %d%%" % data.humidity
  print "Barometer         %.2f inHg" % data.pressure
  print "Solar             %d w/m^2" % data.solarRad
  print "UV                %d uvI" % data.uv
  print "Rain (total/rate) %.2f in / %.2f in/h" % (data.rainTotal,
    data.rainRate)

  if data.windSpeed is None or data.windSpeed == 0:
    print "Wind              calm"
  else:
    print "Wind              %03d deg (%s) @ %d mph" % \
      (data.windDir, winddir(data.windDir), data.windSpeed)

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

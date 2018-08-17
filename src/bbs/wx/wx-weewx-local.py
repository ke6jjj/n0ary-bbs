#!/usr/bin/env python
import time
import math
import collections
import sqlite3

def do_summary(db):
  dbh = sqlite3.connect(db)
  curs = dbh.cursor()

  now = int(time.time())
  now_sub_hour = now % 3600
  next_hour = now + 3600 - now_sub_hour
  day_ago = next_hour - 86400

  def the_triple_query(field):
    return 'min({f}), avg({f}), max({f})'.format(f=field)

  def the_triple_name(field):
    return '{f}_min {f}_avg {f}_max'.format(f=field)

  fields = [ 'barometer', 'outTemp', 'outHumidity', 'windSpeed', 'windDir',
    'radiation', 'UV', 'rain' ]

  curs.execute('SELECT min(dateTime), ' + ','.join(
    [the_triple_query(x) for x in fields]) +
    ' FROM archive WHERE dateTime <= ? AND dateTime >= ? '
    'GROUP BY dateTime / 3600', (now, day_ago)
  )
    
  wx_row = collections.namedtuple('wx_row', 'when ' +
    ' '.join([ the_triple_name(x) for x in fields ]))

  rows = curs.fetchall()

  print len(rows)
  for row in rows:
    data = wx_row(*row)
    # Adjust some items
    data = data._replace(
      # Adjust barometer to integer
      barometer_min=int(data.barometer_min * 100),
      barometer_avg=int(data.barometer_avg * 100),
      barometer_max=int(data.barometer_max * 100),
      # Adjust rain to integer
      rain_max=int(data.rain_max * 100),
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
        data.UV_min,
        data.UV_avg,
        data.UV_max,
        data.rain_max
        ]
      )
    )
       
def winddir(deg):
  quad = int((deg + 22.5) / 45.0)
  quad = quad % 8
  return ('N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW' )[quad]

def do_current(db):
  dbh = sqlite3.connect(db)
  curs = dbh.cursor()
  curs.execute('SELECT dateTime, outTemp, outHumidity, barometer, windSpeed, '
    'windDir, radiation, UV, rain, rainRate FROM archive ORDER BY dateTime '
    'DESC LIMIT 1')
  row = curs.fetchone()

  data = collections.namedtuple('Weather',
    ['when','temperature','humidity','pressure','windSpeed','windDir',
     'solarRad', 'uv', 'rainTotal','rainRate'])(*row)

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
  if len(args) < 3:
    print >>sys.stderr, "usage: wx <sqlite-db> current|summary"
    return 1

  db = args[1]

  if args[2] == 'current':
    do_current(db)
  elif args[2] == 'summary':
    do_summary(db)
  else:
    print >>sys.stderr, "unknown verb"
    return 1

  return 0

if __name__ == '__main__':
  import sys

  sys.exit(main(sys.argv))

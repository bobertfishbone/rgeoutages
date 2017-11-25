#!/usr/bin/python
# vim: set fileencoding=utf-8 :

import math
import os
import sqlite3
import sys
import time
import urllib
import urllib2

from datetime import datetime

try:
    import json
except:
    import simplejson as json

try:
    import secrets
except:
    sys.stderr.write("You need to create a secrets.py file with a Google Maps API key.")
    sys.exit(1)

import scrape_rge


def fetchGeocode(location):
    """Fetches geocoding information.

    Returns dictionary of formattedaddress, latitude, longitude,
    locationtype, and viewport tuple of (sw_lat, sw_lng, ne_lat, ne_lng).
    """

    sanelocation = urllib.quote(location)

    response = urllib2.urlopen("http://maps.googleapis.com/maps/api/geocode/json?address=%s&sensor=false" % sanelocation)

    jsondata = response.read()
    jsondict = json.loads(jsondata)

    if jsondict['results'] == []:
        raise Exception("Empty results string: " + jsondict['status'])

    data = jsondict['results'][0]
    
    viewport = (    data['geometry']['viewport']['southwest']['lat'],
                    data['geometry']['viewport']['southwest']['lng'],
                    data['geometry']['viewport']['northeast']['lat'],
                    data['geometry']['viewport']['northeast']['lng']    )
    outdict = { 'formattedaddress': data['formatted_address'],
                'latitude': data['geometry']['location']['lat'],
                'longitude': data['geometry']['location']['lng'],
                'locationtype': data['geometry']['location_type'],
                'viewport': viewport    }

    time.sleep(2)

    return outdict


def geocode(town, location, street):
    """Geocodes a location, either using the cache or the Google.

    Returns dictionary of formattedaddress, latitude, longitude,
    locationtype, and viewport tuple of (sw_lat, sw_lng, ne_lat, ne_lng).
    """

    town = town.lower().strip()
    if location:
        location = location.lower().strip()
    else:
        location = town
    street = street.lower().strip()

    # disambiguate lanes
    if street.endswith(' la'):
        street += 'ne'

    using_cache = False

    if not using_cache:
        fetchresult = fetchGeocode(street + ", " + location + " NY")
        return fetchresult

def rowcalc(latitude):
    row = latitude - 2
    return row

def columncalc(longitude):
    column = longitude - 42
    return column

def tominutes(latlong):
    if latlong > 0:
        remainder = latlong - math.floor(latlong)
    else:
        remainder = latlong + (math.floor(latlong) * -1)
    #sys.stdout.write('\n'+str(remainder)+'\n')
    if latlong > 0:
        converted = math.floor((remainder * 60))
    else:
        converted = 60 - math.floor((remainder * 60))

    return converted

def arraycalc(row, column):
    rowstring = row * 26
    columnstring = rowstring + column

    return int(columnstring)


if __name__ == '__main__':

    try:
        apikey = secrets.apikey
    except:
        apikey = 'FIXME FIXME FIXME'

    localelist = []
    markerlist = []
    citycenterlist = []
    pointlist = []
    custlist = []

    stoplist = ['HONEOYE%20FL', 'HONEOYE', 'N%20CHILI']

    try:

        historyfd = open('history.json','r')
        historydict = json.load(historyfd)
        historyfd.close()
    except IOError:
        historydict = {}
    newhistorydict = {}
    newjsondict = {}
    array = []
    for i in range(26*13):
        array.append(0)

    # fetch the outages
    outagedata = scrape_rge.crawl_outages()
    #sys.stdout.write(str(outagedata))
    for county, countydata in outagedata.items():
        newjsondict[county] = {}
        towns = countydata['Towns']
        for town, towndata in towns.items():
            newjsondict[county][town] = {}
            locations = towndata['Locations']
            count = 0
            for location, locationdata in locations.items():
                streets = locationdata['Streets']
                newjsondict[county][town][location] = {}
                for street, streetdata in streets.items():
                    newjsondict[county][town][location][street] = {}
                    for key, value in streetdata.items():
                        newjsondict[county][town][location][street][key] = value
                    try:
                        streetinfo = geocode(town, location, street)
                        if streetinfo['formattedaddress'] in historydict.keys():
                            firstreport = historydict[streetinfo['formattedaddress']]
                        else:
                            firstreport = time.time()
                        if streetinfo['locationtype'] == 'APPROXIMATE':
                            streetinfo['formattedaddress'] = '%s? (%s)' % (street, streetinfo['formattedaddress'])
                        newjsondict[county][town][location][street]['geo'] = streetinfo
                        newjsondict[county][town][location][street]['firstreport'] = firstreport
                        pointlist.append(streetinfo)
                        streetinfo['CustomersWithoutPower'] = streetdata['CustomersWithoutPower']
                        newhistorydict[streetinfo['formattedaddress']] = firstreport
                        #sys.stdout.write(str(tominutes(streetinfo['latitude'])))
                        if str(math.floor(streetinfo['latitude'])) == "43.0" and str(math.ceil(streetinfo['longitude'])) == "-77.0":
                            custlist.append(" - ".join((str(math.floor(streetinfo['latitude'])), str(tominutes(streetinfo['latitude'])), str(math.ceil(streetinfo['longitude'])), str(tominutes(streetinfo['longitude'])), str(newjsondict[county][town][location][street]['CustomersWithoutPower']))))
                            arraynum = arraycalc(tominutes(streetinfo['latitude']), tominutes(streetinfo['longitude']))
                            if arraynum <= 337:
                                array[arraynum] += newjsondict[county][town][location][street]['CustomersWithoutPower']
                        count += 1
                    except Exception, e:
                        sys.stdout.write("<!-- Geocode fail: %s in %s gave %s -->\n" % (street, town, e.__str__()))

            if count > 1:
                s = 's'
            else:
                s = ''

            localestring = '<strong>%s</strong>:&nbsp;%i&nbsp;street%s' % (town, count, s)
            for key, value in towndata.items():
                if type(value) is not dict:
                    localestring += ',&nbsp;%s:&nbsp;%s' % (key, value)
            localestring += '&nbsp;(%.2f%%&nbsp;affected)' % (float(towndata['CustomersWithoutPower']) / float(towndata['TotalCustomers']) * 100.0)
            localelist.append(localestring)


    #sys.stdout.write(str(newjsondict))
    # Save json dump file
    #newjsonfd = open('data.new.json','w')
    #json.dump(newjsondict, newjsonfd)
    #newjsonfd.close()
    #os.rename('data.new.json', 'data.json')

    custlistfd = open('custlist.txt', 'w')
    custlistfd.write(' '.join(str(e) for e in array))
    #for line in custlist:
    #    custlistfd.write(line+'\n')

    custlistfd.close()

    for val in range(0, (26*13)):
        sys.stdout.write(str(array[val])+" ")



    if len(markerlist) == 1:
        s = ''
    else:
        s = 's'

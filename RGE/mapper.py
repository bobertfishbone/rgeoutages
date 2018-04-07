#!/usr/bin/python

import math
import os
import sys
import time
import urllib
import urllib2

from datetime import datetime

import sqlite3
conn = sqlite3.connect('rge.db')
c = conn.cursor()


min_lat = {'degrees': 43, 'minutes': 2}
max_long = {'degrees': -77, 'minutes': 24}
max_lat = {'degrees': 43, 'minutes': 15}
min_long = {'degrees': -77, 'minutes': 49}

rows = 13
columns = 26

globalTotalOut = 0

try:
    import json
except:
    import simplejson as json

try:
    import secrets
except:
    sys.stderr.write("You need to create a secrets.py file!")
    sys.exit(1)

import scrape_rge

def fetchGeocode(location, town, street):
    """Fetches geocoding information.

    Returns dictionary of formattedaddress, latitude, longitude,
    locationtype, and viewport tuple of (sw_lat, sw_lng, ne_lat, ne_lng).
    """

    sanelocation = urllib.quote(location)

    response = urllib2.urlopen("https://maps.googleapis.com/maps/api/geocode/json?address=%s&key=%s&sensor=false" % (sanelocation, apikey))

    jsondict = json.load(response)
    #print (response[1])
    if jsondict['results'] == []:
        jsondict['results'] == [""]
        #raise Exception("Empty results string: " + jsondict['status'])

    if jsondict['results'] != []:
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

        newdict = {'latitude': data['geometry']['location']['lat'],
                    'longitude': data['geometry']['location']['lng']}

        time.sleep(1)

        c.execute('''INSERT INTO locations (location, street, lat, long) VALUES (?, ?, ?, ?)''', (town, street, newdict['latitude'], newdict['longitude']))
        conn.commit()
        return newdict

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

    c.execute('''SELECT * FROM locations where location = ? and street = ?''', (location,street))
    row = c.fetchone()

    if  row == None:
        print("Looking up on Googs!")
        fetchresult = fetchGeocode(street + ", " + location + " NY", location, street)
    else:
        print("Pulling from DB!!")
        fetchresult =   {'latitude': row[3],
                    'longitude': row[4]}  

    
    return fetchresult

def tominutes(latlong):
    """The only thing we care about here are minutes--lat and long
    remain the same"""
    if latlong > 0:
        remainder = latlong - math.floor(latlong)
    else:
        remainder = latlong + (math.floor(latlong) * -1)
    #sys.stdout.write('\n'+str(remainder)+'\n')
    if latlong > 0:
        converted = math.floor((remainder * 60))
    else:
        converted = 60 - math.ceil((remainder * 60))

    return int(converted)


def writeArray(lat, long):
    count = 0
    for i in xrange (0, 12):
        for j in xrange (0, 25):

            count += 1    

    return 0

def getRow(minutes):
    row = (max_lat['minutes'])- minutes
    return row

def getColumn(minutes):
    column = min_long['minutes'] - minutes
    return column

if __name__ == '__main__':

    """ Limits of map """


    grid = {}
    array = []
    for i in range(columns * rows + 1):
        array.append(0)
    
    try:
        apikey = secrets.apikey
    except:
        apikey = 'fixme!'

    stoplist = ['HONEOYE%20FL', 'HONEOYE', 'N%20CHILI']

    outagedata = scrape_rge.crawl_outages()

    for county, countydata in outagedata.items():
        if county == u'MONROE':
            towns = countydata['Towns']

            for town, towndata in towns.items():
                locations = towndata['Locations']

                for location, locationdata in locations.items():
                    streets = locationdata['Streets']
                    streetCounter = 0
                    for street, streetdata in streets.items():
                        print streetCounter
                        streetinfo = geocode(town, location, street)
                        if streetinfo is not None:
                            latitude = tominutes(streetinfo['latitude'])
                            longitude = tominutes(streetinfo['longitude'])
                            # Convert minutes from coordinates to 
                            # row/column on the 
                            if latitude >= min_lat['minutes'] and latitude <= max_lat['minutes'] \
                            and longitude >= max_long['minutes'] and longitude <= min_long['minutes']:
                                curRow = getRow(latitude)
                                curCol = getColumn(longitude)
                                ledNum = 0
                                # Fix weird orientation cause I'm extra dumb at soldering
                                #if curRow == 0 or curRow%2 == 0:
                                ledNum = ((curRow - 1) * columns) + (columns - curCol)
                                #else:
                                #    ledNum = ((curRow - 1) * columns) + columns
                                print ("Latitude: "+ str(streetinfo['latitude']))
                                print ("Latitude Minutes: "+str(latitude))
                                print ("Longitude: "+ str(streetinfo['longitude']))
                                print ("Longitude Minutes: "+str(longitude))
                                print ("Current Row: "+str(curRow))
                                print ("Current Column: "+str(curCol))
                                print ("Current LED: "+str(ledNum))
                                outageCust = float(streetdata['CustomersWithoutPower'])
                                globalTotalOut += outageCust
                                totalCust = float(streetdata['TotalCustomers'])
                                percentageOut = int(math.floor((outageCust/totalCust) * 100))
                                print( "LED number: "+ str(ledNum))
                                if array[ledNum] > 0:
                                    array[ledNum] = (array[ledNum] + percentageOut) / 2
                                else:
                                    array[ledNum] = percentageOut
                                
                        streetCounter += 1

    
    custlistfd = open('custlist.txt', 'w')
    custlistfd.write(' '.join(str(e) for e in array))
    custlistfd.write(' '+str(int(globalTotalOut))+' ')
    custlistfd.close()
    

    for val in range(0, len(array)):
        sys.stdout.write(str(array[val])+" ")

conn.close()




                        

                        
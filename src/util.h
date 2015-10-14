#ifndef SIMPLE_ROUTE_UTIL_H
#define SIMPLE_ROUTE_UTIL_H

/***
 * All lat/lon calculations are based on the formulas from http://www.movable-type.co.uk/scripts/latlong.html
 */

namespace simpleroute {

///initial bearing in degrees
double bearingTo(double lat0, double lon0, double lat1, double lon1);

///Cross-track distance: minimum distance between a point on the great-circle arc of p0->p1 and q
double crossTrackDistance(double lat0, double lon0, double lat1, double lon1, double latq, double lonq);

double distanceTo(double lat0, double lon0, double lat1, double lon1, double earthRadius = 6371000.0);

}//end namespace

#endif
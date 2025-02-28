#include <math.h>
#include <stdio.h>
#include "coordinate.h"

//using namespace std;
#ifndef M_PI
#define M_PI 3.1415926535897932384626
#endif
#ifndef M_E
#define M_E 0.00669342162296594323
#endif
#ifndef M_A
#define M_A 6378245.0
#endif
#ifndef x_M_PI
#define x_M_PI  (3.1415926535897932384626 * 3000.0 / 180.0)
#endif




#define PI 3.1415926535897932384626
double baiduFactor = (PI * 3000.0) / 180.0;




static double transformLat(double lat, double lon)
{
    double tlat = -100.0 + 2.0 * lon + 3.0 * lat + 0.2 * lat * lat + 0.1 * lon * lat + 0.2 * pow(fabs(lon), 0.5);
    tlat = tlat + ((20.0 * sin(6.0 * lon * M_PI) + 20.0 * sin(2.0 * lon * M_PI)) * 2.0 / 3.0);
    tlat = tlat + ((20.0 * sin(lat * M_PI) + 40.0 * sin(lat / 3.0 * M_PI)) * 2.0 / 3.0);
    tlat = tlat + ((160.0 * sin(lat / 12.0 * M_PI) + 320 * sin(lat * M_PI / 30.0)) * 2.0 / 3.0);
    return tlat;
}

static double transformLon(double lat, double lon)
{
   double tlon = 300.0 + lon + 2.0 * lat + 0.1 * lon * lon + 0.1 * lon * lat + 0.1 * pow(fabs(lon), 0.5);
   tlon = tlon + ((20.0 * sin(6.0 * lon * M_PI) + 20.0 * sin(2.0 * lon * M_PI)) * 2.0 / 3.0);
   tlon = tlon + ((20.0 * sin(lon * M_PI) + 40.0 * sin(lon / 3.0 * M_PI)) * 2.0 / 3.0);
   tlon = tlon + ((150.0 * sin(lon / 12.0 * M_PI) + 300.0 * sin(lon / 30.0 * M_PI)) * 2.0 / 3.0);
   return tlon;
}
static void transformLoc(double *lat, double *lon)
{
    double tlat = transformLat(*lat - 35.0, *lon - 105.0);
    double tlon = transformLon(*lat - 35.0, *lon - 105.0);
    double ratlat = *lat / 180.0 * M_PI;
    double magic = sin(ratlat);
    magic = 1 - M_E * magic * magic;
    double sqrtmagic = pow(magic, 0.5);
    tlat = (tlat * 180.0) / ((M_A * (1 - M_E)) / (magic * sqrtmagic) * M_PI);
    tlon = (tlon * 180.0) / (M_A / sqrtmagic * cos(ratlat) * M_PI);
    *lat = *lat + tlat;
    *lon = *lon + tlon;
}

void s_Gcj02ToWgs84(LOCDATA *loc)
{
   double lat = loc->lat;
   double lon = loc->lon;
   transformLoc(&lat, &lon);
   loc->lat = loc->lat * 2 - lat;
   loc->lon = loc->lon * 2 - lon;
}
void Gcj02ToWgs84(double *lat, double *lon)
{
   double templat = *lat;
   double templon = *lon;
   transformLoc(&templat, &templon);
   *lat = *lat * 2 - templat;
   *lon = *lon * 2 - templon;
}
void s_Wgs84ToGcj02(LOCDATA *loc)
{
   double lat = loc->lat;
   double lon = loc->lon;
   transformLoc(&lat, &lon);
   loc->lat = lat;
   loc->lon = lon;
}
void s_Gcj02ToBd09(LOCDATA *loc)
{
    double lat = loc->lat;
    double lon = loc->lon;
    double x = lon;
    double y = lat;
    double z = sqrt(x * x + y * y) + 0.00002 * sin(y * x_M_PI);
    double theta = atan2(y, x) + 0.000003 * cos(x * x_M_PI);
    loc->lon = z * cos(theta) + 0.0065;
    loc->lat = z * sin(theta) + 0.006;
}
void s_Wgs84ToBd09(LOCDATA *loc)
{
    s_Wgs84ToGcj02(loc);
    s_Gcj02ToBd09(loc);
}
void Wgs84ToGcj02(double *lat, double *lon)
{
   transformLoc(lat, lon);
}
void Gcj02ToBd09(double *lat, double *lon)
{
    double x = *lon;
    double y = *lat;
    double z = sqrt(x * x + y * y) + 0.00002 * sin(y * x_M_PI);
    double theta = atan2(y, x) + 0.000003 * cos(x * x_M_PI);
    *lon = z * cos(theta) + 0.0065;
    *lat = z * sin(theta) + 0.006;
}
void Wgs84ToBd09(double *lat, double *lon)
{
    Wgs84ToGcj02(lat, lon);
    Gcj02ToBd09(lat, lon);
}
 void Bd09ToGcj02(double *lat, double *lon)
 {
     double x = *lon - 0.0065;
     double y = *lat - 0.006;
     double z = sqrt(x * x + y * y) - 0.00002 * sin(y * x_M_PI);
     double theta = atan2(y, x) - 0.000003 * cos(x * x_M_PI);
     *lon = z * cos(theta);
     *lat = z * sin(theta);
 }
void Bd09ToWgs84(double *lat, double *lon)
{
    Bd09ToGcj02(lat, lon);
    Gcj02ToWgs84(lat, lon);
}

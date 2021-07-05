
#ifndef SATELLITE_H
#define SATELLITE_H

#define PI  3.141592653589793238462643383279
#define CENTURY 2000
#include <norad.h>

#define radial_vel_offset_sec 5  //was time_offset
//double time_offset = 5;

typedef struct {
    float ra;
    float dec;
} equa_pos_t;

typedef struct {
    char name[16];
    float ra;
    float dec;
    float mag;
} star_t;


typedef struct {
    float lat;   //degrees
    float lon;   //degrees
    float alt;   //meters MSL
} topo_pos_t;

typedef struct {
    double jd;  //julian day/time stamp, really should be a double (or broken into a day and fractional day)
    //could also commit fully to j2k throughout, if i were so inclined...
    float  az;   //degrees -- could be a short float
    float  elev; //degrees -- could be a short float
    float  dist; //meters  -- could be a short float
} sat_pos_t;

typedef struct {
    //doesn't include observer because it can only be generated with data from observer, so calcSat caller keeps track of it
    double az; //azimuth from observer
    double elev; //elevation
    double dist;//distance

    double ra;  //right ascension
    double dec; //declination

    double jd;  //time of position
    int satid; //catalog number of the satellite
    int ok;     // ==0 if all is okay, so make sure to check this
} sat_calc_t;

typedef struct {
    sat_pos_t rise; //satellite position when it comes over horizon (elev - to +)
    sat_pos_t max;  //when it's at max elevation
    sat_pos_t set;  //when it goes over horizon (elev + to -)
} sat_pass_t;

typedef struct {
    char       name[16];
    int        channel_idxs[8];
    tle_t      tle;
    sat_calc_t  current;
    sat_pass_t nextpass;
} sat_sat_t;

typedef struct {
    topo_pos_t observer;
} sat_state_t;

extern sat_sat_t satellites[];
extern int num_satellites;
extern star_t stars[];
extern int num_stars;


double curTime_to_julian_day(curTime_t t);
topo_pos_t getObserverPosition();
curTime_t julian_day_to_curTime( double jd );
double jd_to_j2k(double jd);
double local_sidereal_degrees(double j2k, double longitude );
double hour_angle_degrees( double local_sidereal_time, double right_ascension);
void ra_dec_to_az_alt(double jd,
                      double latitude, double longitude,
                      double ra, double dec,
                      double * az_out, double * alt_out);
sat_calc_t  calcSatNow( tle_t tle, state_t last_state );
sat_calc_t  calcSat( tle_t tle, double time_jd, topo_pos_t observer_degrees);
point_t azel_deg_to_xy( float az_deg, float elev_deg, float radius);

point_t offset_point( point_t center, int num, ... );

double sat_nextpass(
    //sat in question
    tle_t tle,
    //start time
    double start_jd,
    //how long in decimal days to search from start_jd
    double search_time_days,
    //observer location, in degrees latitude and longitude, and altitude in meters
    topo_pos_t observer
);



//stuff for the "asteroids" "game"
typedef struct {
    float rot;
    float spdx;
    float spdy;
    float x;
    float y;
} game_obj_2d_t;

void game_move(game_obj_2d_t * o, unsigned long long td);
void game_addvel( game_obj_2d_t * o, float vel, float rot );
void game_obj_init( game_obj_2d_t * o );
void game_obj_screenwrap( game_obj_2d_t * o );
#define DEG(x) ( x*180/PI )
#define RAD(x) ( x*PI/180 )

#endif

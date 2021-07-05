#include <stdio.h>
#include <stdlib.h>

#include <math.h>
#include <string.h>

#include <observe.h>

#include <ui.h>
#include <satellite.h>
#include <watdefs.h>
#include <afuncs.h>


double curTime_to_julian_day(curTime_t t)
{
    //TODO this is suspect and must be proven.
    
    //expects t to be after year 2000
    //many thanks to Peter Baum, and his "Date Algorithms" reference.
    uint8_t s = t.second; //.day is the _weekday_, date is day-of-month
    uint8_t m = t.minute; //.day is the _weekday_, date is day-of-month
    uint8_t h = t.hour; //.day is the _weekday_, date is day-of-month
    uint8_t D = t.date; //.day is the _weekday_, date is day-of-month
    uint8_t M = t.month;
    short Y = CENTURY + t.year;
    /*printf("%04d/%02d/%02d  %02d:%02d:%02d\n", Y,M,D,h,m,s);*/

    int Z = Y + (M - 14) / 12; //relies on int truncation
    const short Fvec[] = {306, 337, 0, 31, 61, 92, 122, 153, 184, 214, 245, 275};
    short F = Fvec[M - 1];

    //! note difference between floor and int truncation
    //floor(-1.5) => -2
    //int(-1.5) => -1
    double jd = D + F + 365 * Z + floor(Z / 4) - floor(Z / 100) + floor(Z / 400) + 1721118.5;
    //that +.5 is because julian .0 is actually _noon_, not midnight
    //so JD #.5 is _halfway through that julian day_, at _midnight_
    //so we add hours, minutes, and seconds into a fractional day and add that to our new "midnight" epoch and...
    jd += ((float)h) / 24 + ((float)m) / 1440 + ((float)s) / 86400;
    //voila!

    /*printf("jd: %.6f\n",jd);*/
    return jd;
}
/*
   void test_curTime_to_julian_day(){
   curTime_t t;
   }
   curTime_t julian_day_to_curTime( double jd ){
//expects jd to be after year 2000
curTime_t t;
long Z = jd - 1721118.5;
double R = jd - 1721118.5 - Z;
double G = Z - .25;
//TODO unfinished:
//
A = INT(G / 36524.25)
B = A -INT(A / 4)
year = INT((B+G) / 365.25)
C = B + Z -INT(365.25 * year)
month = FIX((5 * C + 456) / 153)
day = C -FIX((153 * month -457) / 5) + R
if(month>12){year++;month-=12;}
//

t.second = 0;
t.minute = 0;
t.hour = 0;

t.date = D;
t.month = M;
t.year = 2000-Y;
return t;

}
*/
double jd_to_j2k(double jd)
{
    return jd - 2451545.0;
}
double wrap( double in, double max)
{
    //TODO replace with fmod i think
    while( in < 0 ) {
        in += max;
    }
    while( in >= max ) {
        in -= max;
    }
    return in;
}
double local_sidereal_degrees(double j2k, double longitude )
{
    double UT = 24 * (j2k - floor(j2k) + .5);
    /*printf("J2K: %.6f \n", j2k);*/
    /*printf("UT: %.6f \n", UT);*/
    double degrees_rotation_per_day = .985647;
    double gmst_j2k_correction = 100.46;
    double lond = DEG(longitude);
    double local_sidereal_time = gmst_j2k_correction + degrees_rotation_per_day * j2k + lond + 15 * UT;
    local_sidereal_time = wrap(local_sidereal_time, 360);
    return local_sidereal_time;
}
double hour_angle_degrees( double local_sidereal_time, double right_ascension)
{
    double ha = local_sidereal_time - right_ascension;
    ha = wrap( ha, 360 );
    return ha;
}

void ra_dec_to_az_alt(double jd,
                      double latitude, double longitude,
                      double ra, double dec,
                      double * az_out, double * alt_out)
{
    if( 1 ){
        double j2k = jd_to_j2k(jd);
        double lst = local_sidereal_degrees(j2k, longitude); //✔
        /*printf("lst %.4f\n", lst);*/
        double ha = RAD(hour_angle_degrees(lst, DEG(ra) ));

        latitude = latitude;

        double A = cos(ha) * cos(dec) * cos(latitude) - sin(dec) * sin(latitude);
        double B = sin(ha) * cos(dec);
        double C = cos(ha) * cos(dec) * sin(latitude) + sin(dec) * cos(latitude);
        double az = atan2(B, A) + PI;
        double alt = asin(C);

        az = wrap(az, 2 * PI);

        *az_out = az;
        *alt_out = alt;
        return;
    } else {
        //replace with lunar alt_az.cpp functions
        DPT radec = {ra, dec};
        DPT altaz = {0};
        DPT latlon = {latitude, longitude};
        full_ra_dec_to_alt_az(
                &radec, // DPT *ra_dec,
                &altaz, //DPT *alt_az,
                NULL, //DPT *loc_epoch, 
                &latlon, //DPT *latlon,
                jd, //double jd_utc, 
                NULL //double *hr_ang
                );
        *alt_out = altaz.x;
        *az_out = altaz.y;
    }
}


topo_pos_t getObserverPosition(){
    topo_pos_t obs;
    if( ! last_state.settings.gps_enabled || last_state.gps_data.fix_quality == 0 ){
        //fix_type is 1 sometimes when it shouldn't be, have to use fix_quality

        //TODO: need a way to show gps enabled/disable, gps fix/nofix
        //gfx_print(layout.line3_pos, "no gps fix", FONT_SIZE_12PT, TEXT_ALIGN_CENTER, color_white);

        //TODO pull from manual position data rather than hardcoding
        obs.lat =  41.70011;
        obs.lon = -70.29947;
        obs.alt = 0; //msl geoid meters
    } else {
        obs.lat = last_state.gps_data.latitude;
        obs.lon = last_state.gps_data.longitude;
        obs.alt = last_state.gps_data.altitude; //msl geoid meters
    }
    return obs;
}
sat_calc_t calcSatNow( tle_t tle, state_t last_state ){
    double jd;
    topo_pos_t obs = getObserverPosition();
    jd = curTime_to_julian_day(last_state.time);
    return calcSat(tle, jd, obs);
}
sat_calc_t  calcSat( tle_t tle, double time_jd, topo_pos_t observer_degrees)
{
    topo_pos_t obs = { //observer_degrees, but in radians
        RAD(observer_degrees.lat),
        RAD(observer_degrees.lon),
        observer_degrees.alt
    };
    int is_deep = select_ephemeris( &tle );
    int ephem = 1;       /* default to SGP4 */
    double sat_params[N_SAT_PARAMS], observer_loc[3];
    double rho_sin_phi;
    double rho_cos_phi;

    double ra;
    double dec;
    double dist_to_satellite;

    double t_since;
    double pos[3];
    //double vel[3]; //pass in place of the NULL after pos to get velocities
    int err_val = 0;

    //remember to put lat and lon in rad by this point
    earth_lat_alt_to_parallax( obs.lat, obs.alt, &rho_cos_phi, &rho_sin_phi);
    observer_cartesian_coords( time_jd, obs.lon, rho_cos_phi, rho_sin_phi, observer_loc);
    if( is_deep && (ephem == 1 || ephem == 2)) {
        ephem += 2;
    }
    if( !is_deep && (ephem == 3 || ephem == 4)) {
        ephem -= 2;
    }

    t_since = (time_jd - tle.epoch) * 1440;
    switch( ephem) {
    case 0:
        SGP_init( sat_params, &tle);
        err_val = SGP( t_since, &tle, sat_params, pos, NULL);
        break;
    case 1:
        SGP4_init( sat_params, &tle);
        err_val = SGP4( t_since, &tle, sat_params, pos, NULL);
        break;
    case 2:
        SGP8_init( sat_params, &tle);
        err_val = SGP8( t_since, &tle, sat_params, pos, NULL);
        break;
    case 3:
        SDP4_init( sat_params, &tle);
        err_val = SDP4( t_since, &tle, sat_params, pos, NULL);
        break;
    case 4:
        SDP8_init( sat_params, &tle);
        err_val = SDP8( t_since, &tle, sat_params, pos, NULL);
        break;
    default:
        printf( "? How did we get here? ephem = %d\n", ephem);
        err_val = 0;
        break;
    }
    if( err_val ) {
        printf( "Ephemeris error %d\n", err_val);
    }
    get_satellite_ra_dec_delta( observer_loc, pos, &ra, &dec, &dist_to_satellite);
    epoch_of_date_to_j2000( time_jd, &ra, &dec);
    double az = 0;
    double elev = 0;
    ra_dec_to_az_alt(time_jd, obs.lat, obs.lon, ra, dec, &az, &elev);
    /*printf("POS: %.4f,%.4f,%.4f\n", pos[0], pos[1], pos[2] );*/
    /*printf("VEL: %.4f,%.4f,%.4f\n", vel[0], vel[1], vel[2] );*/
    sat_calc_t ret;
    ret.az = az;
    ret.elev = elev;
    ret.ra = ra;
    ret.dec = dec;
    ret.dist = dist_to_satellite;
    ret.jd = time_jd;
    ret.satid = tle.norad_number;
    ret.ok = err_val;
    return ret;
}
double sat_nextpass(
    //sat in question
    tle_t tle,
    //start time
    double start_jd,
    //how long in decimal days to search from start_jd
    double search_time_days,
    //observer location, in degrees latitude and longitude, and altitude in meters
    topo_pos_t observer
)
{
    //determine sampling from TLE ?
    //getting a list of passes involves successive calls to this function, incrementing start_jd

    double jd = start_jd;

    int aboveHorizonAtJD = 0;
    int i = 0;
    double coarse_interval = 30.0 / 86400; //30 seconds in decimal days
    double fine_interval = 1.0 / 86400; //30 seconds in decimal days
    //search for when a pass comes over the horizon
    while( jd < start_jd + search_time_days ) { //search next N day(s)
        jd += coarse_interval; //increment by ~30s
        sat_calc_t s = calcSat( tle, jd, observer);
        aboveHorizonAtJD = s.elev >= 0;
        i++;
        if( aboveHorizonAtJD ) {
            //we found a pass!
            break;
        }
    }
    while( aboveHorizonAtJD ) {
        sat_calc_t s = calcSat( tle, jd, observer);
        aboveHorizonAtJD = s.elev >= 0;
        i++;
        //could speed this up by bisect bracketing the transition from - to +
        //should also find the end of the pass going + to -
        jd -= fine_interval; // <1s
    }
    return jd;
}
/*sat_pass_t sat_getpass(){*/
/*}*/
sat_pos_t calc2pos( sat_calc_t c ){
    sat_pos_t p = { c.jd, c.az, c.elev, c.dist };
    return p;
}
/*sat_getpass_points(sat_pass_t pass, int points, sat_pos_t * out){*/
    /*[>pass.rise.jd;<]*/
    /*[>pass.max.jd;<]*/
    /*[>pass.set.jd;<]*/
/*}*/




/*
   struct misc_for_sat_tracking {
   char manual_gridsquare[9]; //to be converted to latlon for positioning when gps is missing or still warming up
//tle_t * tle; //pointer to
//julian day, to be recalculculated every second when rtc is queried
//double jd = curTime_to_julian_day(last_state.time);
//decimal time in hours
//double UT = (double)last_state.time.hour + ((double)last_state.time.minute)/60 + ((double) last_state.time.second)/3600;
};

void x(){
tle_t tle;
//replace this with a system of storing
//tle_t's in codeplug area later
//
//iss
char * line1 = "1 25544U 98067A   21090.24132166  .00001671  00000-0  38531-4 0  9996";
char * line2 = "2 25544  51.6473   8.2465 0003085 162.7840 356.2009 15.48967005276518";

int err_val = parse_elements( line1, line2, &tle );

//printf("JD %.6f\r\n", jd);
//snprintf(sbuf, 25, "JD %.6f", jd);
//gfx_print(layout.line3_pos, sbuf, FONT_SIZE_8PT, TEXT_ALIGN_CENTER, color_white);

//TODO:
//drawing system for illustrating a pass?
//need a satellite selection view
//  and then eventually
//a pass list view
//a pass detail view
//details we care about: most important, time and max elev
//rise time, duration, max elev, start and stop azimuths

double latr = lat*PI/180; //sat code works in radians
double lonr = lon*PI/180; //so r for radians
double ra = 0;
double dec = 0;

double dist = 0;
double ra2 = 0;
double dec2 = 0;
double dist2 = 0;

double toff_jd = time_offset/(24*60*60);
calcSat( tle, jd, latr, lonr, alt, &ra, &dec, &dist);
calcSat( tle, jd + toff_jd, latr, lonr, alt, &ra2, &dec2, &dist2);
float toff_dist = dist-dist2; //km difference
float radial_v = (toff_dist)/time_offset; //km/s
float freq = 433e6; //MHz
int doppler_offset = freq*radial_v/300000; //hz

double az = 0;
double elev = 0;
ra_dec_to_az_alt(jd, latr, lonr, ra, dec, &az, &elev);
//replace with lunars:
//void DLL_FUNC full_ra_dec_to_alt_az( const DPT DLLPTR *ra_dec,
//            DPT DLLPTR *alt_az,
////           DPT DLLPTR *loc_epoch, const DPT DLLPTR *latlon,
//         const double jd_utc, double DLLPTR *hr_ang)

elev *= 180/PI; //radians to degrees
az *= 180/PI;

//double pass = nextpass_jd(tle, jd, lat, lon, alt ); //works in degrees
//int pass_diff = (pass - UT)*3600; //diff is seconds until (+) or since (-) the mark timestamp


double pass_start_jd = 2459306.456495;
double pass_duration_jd = .025; //~90s
double pass_end_jd = pass_start_jd + pass_duration_jd;

char * pass_state;
double mark = pass_start_jd;
//mark is the time we care about most - e.g. before a pass, it's the time the pass starts
//or during a pass, it's the LOS time (when the sat will go below the horizon)
if( UT < pass_start_jd ){
    //before this pass comes over the horizon
    pass_state = "AOS";
    mark = pass_start_jd;
} else if ( UT > pass_start_jd && UT < pass_end_jd ){
    //during the pass
    pass_state = "LOS";
    mark = pass_end_jd;
} else {
    //now it's gone over the horizon, so same as the elif above (just will be a
    //negative number to show it was in the past)
    //left here for clarity to show the actual LOS condition
    pass_state = "LOS";
    mark = pass_end_jd;
}
int diff = (mark - UT)*3600; //diff is seconds until (+) or since (-) the mark timestamp
const char * sat_name = "ISS";

if( diff > 60 || diff < -60 ){
    //if we're too far before or after the mark time, it
    //makes more sense to show something else rather than, say, 12387123 seconds
    //therefore print the time of the mark time
    int mark_h = 0;
    int mark_m = 0;
    snprintf(sbuf, 25, "%s %s %02d:%02d", sat_name, pass_state, mark_h, mark_m);
    gfx_print(layout.line3_pos, sbuf, FONT_SIZE_8PT, TEXT_ALIGN_CENTER, color_white);
} else {
    //will look like these examples:
    //"AOS 30s" meaning it'll come over the horizon in 30s
    //or "LOS 90s" meaning it's in your sky now and will be back over the horizon in 90s
    //or "LOS -30s" meaning it went over the horizon 30 seconds ago
    snprintf(sbuf, 25, "%s %s %ds", sat_name, pass_state, diff);
    gfx_print(layout.line3_pos, sbuf, FONT_SIZE_8PT, TEXT_ALIGN_CENTER, color_white);
}

}

skip

double jd = curTime_to_julian_day(last_state.time);
static double jd_offset = 0;
if( jd_offset == 0 ){
    jd_offset = pass_azel[0].jd - jd;
}
jd = jd + jd_offset;
for( int i = 0; i < num_points_pass; i++){
    sat_pos_t p = pass_azel[i];
    if( p.jd <= jd ){
        az = p.az;
        elev = p.elev;
    } else {
        printf("pass[%d]\n", i);
        break;
    }
}

gfx_clearScreen();
//I've been using this to keep an eye on alignment, but remove later
//gfx_drawVLine(SCREEN_WIDTH/2, 1, color_grey);
//gfx_drawHLine(SCREEN_HEIGHT/2, 1, color_grey);
_ui_drawMainBackground();
_ui_drawMainTop();
_ui_drawBottom();

//get a position. This will be used all over the place.
if( ! last_state.settings.gps_enabled || last_state.gps_data.fix_quality == 0 ){
    //fix_type is 1 sometimes when it shouldn't be, have to use fix_quality

    //TODO: need a way to show gps enabled/disable, gps fix/nofix
    //gfx_print(layout.line3_pos, "no gps fix", FONT_SIZE_12PT, TEXT_ALIGN_CENTER, color_white);

    //TODO pull from manual position data rather than hardcoding
    lat =  41.70011;
    lon = -70.29947;
    //alt = 0; //msl geoid meters
} else {
    lat = last_state.gps_data.latitude;
    lon = last_state.gps_data.longitude;
    //alt = last_state.gps_data.altitude; //msl geoid meters
}

// left side
// relative coordinates to satellite
snprintf(sbuf, 25, "AZ %.1f", az);
gfx_print(layout.line1_pos, sbuf, FONT_SIZE_8PT, TEXT_ALIGN_LEFT, color_white);
snprintf(sbuf, 25, "EL %.1f", elev);
gfx_print(layout.line2_pos, sbuf, FONT_SIZE_8PT, TEXT_ALIGN_LEFT, color_white);

//right side
//doppler correction readout
snprintf(sbuf, 25, "%.1fk DOP", ((float)doppler_offset)/1000);
//gfx_print(layout.line1_pos, sbuf, FONT_SIZE_8PT, TEXT_ALIGN_RIGHT, color_white);
//draw gridsquare text
lat_lon_to_maidenhead(lat, lon, gridsquare, 3); //precision=3 here means 6 characters like FN41uq
gfx_print(layout.line1_pos, gridsquare, FONT_SIZE_8PT, TEXT_ALIGN_RIGHT, color_white);

//center bottom - show
//satellite and AOS/LOS countdown
//gfx_print(layout.line3_pos, "ISS AOS 30s", FONT_SIZE_8PT, TEXT_ALIGN_CENTER, color_white);

//draw Az/El
int r1 = SCREEN_WIDTH/2/4; //0 degrees elev
int r2 = r1/2; // 45 degrees
point_t az_center = {SCREEN_WIDTH/2, SCREEN_HEIGHT/2-5};
point_t az_top = {SCREEN_WIDTH/2-1, SCREEN_HEIGHT/2-5-r1+6};
point_t az_right = {SCREEN_WIDTH/2+r1-5, SCREEN_HEIGHT/2-3};
point_t az_left = {SCREEN_WIDTH/2-r1+3, SCREEN_HEIGHT/2-3};
point_t az_bot = {SCREEN_WIDTH/2-1, SCREEN_HEIGHT/2-5+r1};
gfx_drawCircle(az_center, r1, color_grey);
gfx_drawCircle(az_center, r2, color_grey);
gfx_print(az_top,  "N", FONT_SIZE_5PT, TEXT_ALIGN_LEFT, color_grey);
gfx_print(az_right,"E", FONT_SIZE_5PT, TEXT_ALIGN_LEFT, color_grey);
gfx_print(az_left, "W", FONT_SIZE_5PT, TEXT_ALIGN_LEFT, color_grey);
gfx_print(az_bot,  "S", FONT_SIZE_5PT, TEXT_ALIGN_LEFT, color_grey);


point_t plus_ctr_offset_8pt = {-4,2};
point_t relsatpos = azel_deg_to_xy( az, elev, r1);
point_t satpos = offset_point(az_center, 2, plus_ctr_offset_8pt, relsatpos );

gfx_print(satpos,  "+", FONT_SIZE_8PT, TEXT_ALIGN_LEFT, yellow_fab413);

for( int i = 0; i < num_points_pass; i+=2 ){
    point_t offset = azel_deg_to_xy( pass_azel[i].az, pass_azel[i].elev, r1);
    point_t set = offset_point( az_center, 1, offset);
    gfx_setPixel(set, color_white);
}
char * pass_state;
double mark;
double pass_start_jd = pass_azel[0].jd;
double pass_end_jd = pass_azel[num_points_pass-1].jd;
//mark is the time we care about most - e.g. before a pass, it's the time the pass starts
//or during a pass, it's the LOS time (when the sat will go below the horizon)
if( jd < pass_start_jd ){
    //before this pass comes over the horizon
    pass_state = "AOS";
    mark = pass_start_jd;
} else if ( jd > pass_start_jd && jd < pass_end_jd ){
    //during the pass
    pass_state = "LOS";
    mark = pass_end_jd;
} else {
    //now it's gone over the horizon, so same as the elif above (just will be a
    //negative number to show it was in the past)
    //left here for clarity to show the actual LOS condition
    pass_state = "LOS";
    mark = pass_end_jd;
}
float diff = (mark - jd)*86400; //diff is seconds until (+) or since (-) the mark timestamp
//printf("%f\n",diff);
const char * sat_name = "ISS";
snprintf(sbuf, 25, "%s %s %.0fs", sat_name, pass_state, diff);
gfx_print(layout.line3_pos, sbuf, FONT_SIZE_8PT, TEXT_ALIGN_CENTER, color_white);
*/

void game_move(game_obj_2d_t * o, unsigned long long td){
    o->x += (o->spdx)*td/1000;
    o->y += (o->spdy)*td/1000;
}
void game_addvel( game_obj_2d_t * o, float vel, float rot ){
    float vy = vel * sin(rot);
    float vx = vel * cos(rot);
    o->spdx += vx;
    o->spdy += vy;
}
void game_obj_init( game_obj_2d_t * o ){
    o->x = rand() % SCREEN_WIDTH;
    o->y = rand() % SCREEN_HEIGHT;
    o->rot = ((float)(rand() % ((int)(2*PI*100))))/100;
    float vel = (rand() % 10);
    game_addvel( o, vel, o->rot);
}
void game_obj_screenwrap( game_obj_2d_t * o ){
    if( o->x < 0 ){
        o->x = SCREEN_WIDTH + o->x;
    }
    if( o->x >= SCREEN_WIDTH ){
        o->x = o->x - SCREEN_WIDTH;
    }
    if( o->y < 0 ){
        o->y = SCREEN_HEIGHT + o->y;
    }
    if( o->y >= SCREEN_HEIGHT ){
        o->y = o->y - SCREEN_HEIGHT;
    }
}

void init_sat_global(){
    //global satellites, num_satellites
    //
    //Hardcoded TLEs must be updated.
    //This also implies updating TLEs requires a firmware update.
    //This is a temporary measure until nvm is ready.
    tle_t iss_tle = {0};
    char * line1 = "1 25544U 98067A   21098.03858124  .00002197  00000-0  48233-4 0  9994";
    char * line2 = "2 25544  51.6464 329.6848 0002834 196.2120 269.0598 15.48873021277729";
    parse_elements( line1, line2, &iss_tle );
    tle_t ao92_tle = {0};
    line1 = "1 43137U 18004AC  21097.60567794  .00001372  00000-0  57805-4 0  9996";
    line2 = "2 43137  97.4110 172.3573 0010557 100.0964 260.1465 15.25009246179867";
    parse_elements( line1, line2, &ao92_tle );
    tle_t by702_tle = {0};
    line1 = "1 45857U 20042B   21097.77977101  .00001190  00000-0  17671-3 0  9991";
    line2 = "2 45857  97.9857 174.2638 0012834  86.0351 274.2313 14.76828112 41117";
    parse_elements( line1, line2, &by702_tle );


    sat_calc_t empty1 = {0};
    sat_pass_t empty2 = {0};

    sat_sat_t  iss = {"ISS", {1,0,0,0, 0,0,0,0}, iss_tle, empty1, empty2 };
    sat_sat_t  ao92 = {"AO-92", {2,0,0,0, 0,0,0,0}, ao92_tle, empty1, empty2 };
    /*sat_sat_t  by702 = {"BY70-2", {3,0,0,0, 0,0,0,0}, by702_tle, empty1, empty2 };*/
    memcpy(&satellites[0], &iss, sizeof(sat_sat_t));
    memcpy(&satellites[1], &ao92, sizeof(sat_sat_t));
    /*memcpy(&satellites[2], &by702, sizeof(sat_sat_t));*/
    tle_t tle;
    /*line1 = "1 27607U 02058C   21097.75620756 -.00000013  00000-0  18517-4 0  9995";*/
    /*line2 = "2 27607  64.5557 194.5275 0033184  48.5078 311.8864 14.75731425984124";*/
    /*parse_elements( line1, line2, &tle );*/
    /*sat_sat_t  saudisat = {"saudisat 1c", {4,0,0,0, 0,0,0,0}, tle, empty1, empty2 };*/
    /*memcpy(&satellites[3], &saudisat, sizeof(sat_sat_t));*/

    line1 = "1 43192U 18015A   21097.94807139  .00003611  00000-0  13646-3 0  9998";
    line2 = "2 43192  97.4754 233.0410 0015968 128.3940  17.8444 15.27529340176826";
    parse_elements( line1, line2, &tle );
    sat_sat_t  fmn1 = {"FMN1", {5,0,0,0, 0,0,0,0}, tle, empty1, empty2 };
    memcpy(&satellites[2], &fmn1, sizeof(sat_sat_t));

    line1 = "1 07530U 74089B   21097.73155343 -.00000035  00000-0  63996-4 0  9999";
    line2 = "2 07530 101.8477  72.3275 0012304 156.4477  17.2774 12.53648185122865";
    parse_elements( line1, line2, &tle );
    sat_sat_t  oscar7 = {"oscar7", {5,0,0,0, 0,0,0,0}, tle, empty1, empty2 };
    memcpy(&satellites[3], &oscar7, sizeof(sat_sat_t));

    line1 = "1 43678U 18084H   21097.88856453 -.00000230  00000-0 -17049-4 0  9997";
    line2 = "2 43678  97.9114 222.8511 0008587 287.9394  72.0894 14.91724574132912";
    parse_elements( line1, line2, &tle );
    sat_sat_t  po101 = {"PO-101", {5,0,0,0, 0,0,0,0}, tle, empty1, empty2 };
    memcpy(&satellites[4], &po101, sizeof(sat_sat_t));
}
sat_sat_t satellites[5] = {0};
int num_satellites = 5;
star_t stars[] = {
    // { name, right ascension, declination, magnitude }
    {"Polaris",         2+32/60,  89.3,  1.99},
    {"Sirius",          6+45/40, -16.7, -1.46},
    {"Canopus",         6+24/40, -52.7, -0.73},
    {"A. Centauri",    14+40/40, -60.8, -0.29},
    {"Arcturus",       14+16/40,  19.2, -0.05},
    {"Vega",           18+37/40,  38.8,  0.03},
    {"Capella",         5+17/40,  46.0,  0.07},
    {"Rigel",           5+15/40,  -8.2,  0.15},
    {"Procyon",         7+39/40,   5.2,  0.36},
    {"Achernar",        1+38/40, -57.2,  0.45},
    {"Betelgeuse",      5+55/40,   7.4,  0.55},
    {"Hadar",          14+ 4/40, -60.4,  0.61},
    {"Altair",         19+51/40,   8.9,  0.77},
    {"Acrux",          12+27/40, -63.1,  0.79},
    {"Aldebaran",       4+36/40,  16.5,  0.86},
    {"Antares",        16+29/40, -26.4,  0.95},
    {"Spica",          13+25/40, -11.2,  0.97},
    {"Pollux",          7+45/40,  28.0,  1.14},
    {"Fomalhaut",      22+58/40, -29.6,  1.15},
    {"Deneb",          20+41/40,  45.3,  1.24},
    {"Mimosa",         12+48/40, -59.7,  1.26},
    {"Regulus",        10+ 8/40,  12.0,  1.36},
    {"Adhara",          6+59/40, -29.0,  1.50},
    {"Castor",          7+35/40,  31.9,  1.58},
    {"Shaula",         17+34/40, -37.1,  1.62},
    {"Gacrux",         12+31/40, -57.1,  1.63},
    {"Bellatrix",       5+25/60,   6.3,  1.64},
    {"Elnath",          5+26/60,  28.6,  1.66},
    {"Miaplacidus",     9+13/60, -69.7,  1.67},
    {"Alnilam",         5+36/60,  -1.2,  1.69},
    {"Alnair",         22+ 8/60, -47.0,  1.74},
    {"Alnitak",         5+41/60,  -1.9,  1.75},
    {"Alioth",         12+54/60,  56.0,  1.77},
    {"Mirfak",          3+24/60,  49.9,  1.80},
    {"Dubhe",          11+ 4/60,  61.8,  1.80},
    {"Regor",           8+10/60, -47.3,  1.81},
    {"Wezen",           7+ 8/60, -26.4,  1.83},
    {"Kaus Australis", 18+24/60, -34.4,  1.84},
    {"Alkaid",         13+48/60,  49.3,  1.86},
    {"Sargas",         17+37/60, -43.0,  1.86},
    {"Avior",           8+23/60, -59.5,  1.87},
    {"Menkalinan",      6+ 0/60,  44.9,  1.90},
    {"Atria",          16+49/60, -69.0,  1.92},
    {"Alhena",          6+38/60,  16.4,  1.93},
    {"Peacock",        20+26/60, -56.7,  1.93},
    {"Koo She",         8+45/60, -54.7,  1.95},
    {"Mirzam",          6+23/60, -18.0,  1.98},
    {"Alphard",         9+28/60,  -8.7,  1.98},
    {"Polaris",         2+32/60,  89.3,  1.99},
    {"Algieba",        10+20/60,  19.8,  2.00},
    {"Hamal",           2+ 7/60,  23.5,  2.01},
    {"Diphda",          0+44/60, -18.0,  2.04},
    {"Nunki",          18+55/60, -26.3,  2.05},
    {"Menkent",        14+ 7/60, -36.4,  2.06},
    {"Alpheratz",       0+ 8/60,  29.1,  2.07},
    {"Mirach",          1+10/60,  35.6,  2.07},
    {"Saiph",           5+48/60,  -9.7,  2.07},
    {"Kochab",         14+51/60,  74.2,  2.07},
    {"Al Dhanab",      22+43/60, -46.9,  2.07},
    {"Rasalhague",     17+35/60,  12.6,  2.08},
    {"Algol",           3+ 8/60,  41.0,  2.09},
    {"Almach",          2+ 4/60,  42.3,  2.10},
    {"Denebola",       11+49/60,  14.6,  2.14},
    {"Cih",             0+57/60,  60.7,  2.15},
    {"Muhlifain",      12+42/60, -49.0,  2.20},
    {"Naos",            8+ 4/60, -40.0,  2.21},
    {"Aspidiske",       9+17/60, -59.3,  2.21},
    {"Alphecca",       15+35/60,  26.7,  2.22},
    {"Suhail",          9+ 8/60, -43.4,  2.23},
    {"Mizar",          13+24/60,  54.9,  2.23},
    {"Sadr",           20+22/60,  40.3,  2.23},

};
int num_stars = sizeof( stars ) / sizeof( star_t );

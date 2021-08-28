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
    //t appears to correctly be UTC which we expect, good.
    //this appears to be correct to within at least a second
    
    //expects t to be after year 2000
    //many thanks to Peter Baum, and his "Date Algorithms" reference.
    uint8_t s = t.second; 
    uint8_t m = t.minute; 
    uint8_t h = t.hour; 
    uint8_t D = t.date; //.day is the _weekday_, date is day-of-month
    uint8_t M = t.month;
    short Y = CENTURY + t.year; //CENTURY is from rtc interface
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
double UT_dechrs_from_jd(double jd){
    double UT = 24 * (jd - floor(jd) + .5);
    return UT;
}
double local_sidereal_degrees(double j2k, double longitude )
{
    double UT = UT_dechrs_from_jd(j2k); //hours, naturally
    /*printf("J2K: %.6f \n", j2k);*/
    /*printf("UT: %.6f \n", UT);*/
    double degrees_rotation_per_day = .985647;
    double gmst_j2k_correction = 100.46;
    double local_sidereal_time = gmst_j2k_correction + degrees_rotation_per_day * j2k + longitude + 15 * UT;
    local_sidereal_time = wrap(local_sidereal_time, 360);
    return local_sidereal_time;
}
double hour_angle_degrees( double lst_deg, double ra_hrs)
{
    double ha = lst_deg - HRS2DEG(ra_hrs);
    ha = wrap( ha, 360 );
    //note that hour angle is in range -PI,PI in liblunar...and we're 0,360
    return ha; //hour angle is in degrees
}
void ra_dec_to_alt_az( const double hr_ang, const double dec, double *alt, double *az, const double lat)
{
   double temp, cos_lat = cos( lat);

   *alt = asine( sin( lat) * sin( dec) + cos_lat * cos( dec) * cos( hr_ang));
   if( cos_lat < .00001)         /* polar case */
      *az = hr_ang;
   else
      {   
      temp = (sin( dec) - sin( *alt) * sin( lat)) / (cos( *alt) * cos_lat);
      temp = PI - acose( temp);
      *az = ((sin( hr_ang) < 0.) ? temp : -temp);
      }   
}
void ra_dec_to_az_alt(double jd,
                      double latitude, double longitude,
                      double ra_hrs, double dec_deg,
                      double * az_out_deg, double * alt_out_deg)
{
    if( 1 ){
        double j2k = jd_to_j2k(jd);
        double lst = local_sidereal_degrees(j2k, longitude); 
        /*printf("lst %.4f\n", lst);*/
        double ha = RAD(hour_angle_degrees(lst, ra_hrs )); //radians
        /*printf("HA mine = %f\n", DEG(ha));*/

        double latrad = RAD(latitude);
        double dec = RAD(dec_deg);

        double alt = asin( sin(latrad) * sin(dec) + cos(latrad)*cos(dec)*cos(ha));
        double A = acos(
                (sin(dec) - sin(alt)*sin(latrad))
                /
                (cos(alt)*cos(latrad))
                );
        double az = sin(ha) < 0? A: 2*PI-A;

        az = wrap(az, 2 * PI);

        *az_out_deg = DEG(az);
        *alt_out_deg = DEG(alt);
        return;
    } else {
        /*DPT radec = {RAD(dec_deg), RAD(HRS2DEG(ra_hrs))}; // from looking at precess.cpp, both expected to be in suitable form for sin,cos etc*/

        //full_ra_dec_to_alt_az
        //DPT ra_dec comes in
        //gets pushed to precess_pt, which spits out a DPT loc_at_epoch
        //DPT ra_dec is renamed DPT ipt (in point)
        //temp_ipt[0] = -ipt->x  <-RA
        //temp_ipt[1] = ipt->y <- DEC
        //precess_ra_dec:
        //      where temp_ipt[0] is "old_ra"
        //      so:
        //loc_at_epoch->x = -temp_opt[0] <- RA
        //loc_at_epoch->y = -temp_opt[1] <- DEC
        //when loc_at_epoch is handed to ra_dec_to_alt_az, ra_dec.y is DEC!
        //so DPT radec is correct {ra,dec}
        //
        DPT radec = {RAD(HRS2DEG(ra_hrs)), RAD(dec_deg)}; 
        // from looking at precess.cpp, both expected to be in suitable form for sin,cos etc
            //so needs to be radians
        /*DPT latlon = {RAD(latitude), RAD(longitude)};*/
        DPT latlon = {RAD(longitude), RAD(latitude)}; //x,y; and x is _longitude_! BAD NAMING!
        //BEWARE -- lunar library stores data in DPT
        //BUT!
        //DPT alt_az = {double x, y}
        //and they consistently use it y, then x!
        //which means when you use it, latlon is stored x=lon, y=lat 
        //alt_az is stored alt=y, az=x
        DPT altaz = {0}; 
        //by convention, alt should be (-PI,PI), az should be (0,2*PI)
        //except there is some evidence in liblunar that az is likely to be (-PI,PI)
        //hence the +PI in there at the end
        double j2k = jd_to_j2k(jd);
        double ha;
        full_ra_dec_to_alt_az(
                &radec, // DPT *ra_dec,
                &altaz, //DPT *alt_az,
                NULL, //DPT *loc_epoch, 
                &latlon, //DPT *latlon,
                jd, //double jd_utc, 
                &ha //double *hr_ang
                );
        *alt_out_deg = DEG(altaz.y); 
        *az_out_deg  = DEG(altaz.x +PI);
        /*printf("ha = %f\n", DEG(ha+PI));*/
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
sat_pos_t calcSatNow( tle_t tle, state_t last_state ){
    double jd;
    topo_pos_t obs = getObserverPosition();
    jd = curTime_to_julian_day(last_state.time);
    return calcSat(tle, jd, obs);
}
sat_pos_t  calcSat( tle_t tle, double time_jd, topo_pos_t observer_degrees)
{
    topo_pos_t obs = { //observer position, but in radians
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
        /*printf( "? How did we get here? ephem = %d\n", ephem);*/
        err_val = 0;
        break;
    }
    if( err_val ) {
        /*printf( "Ephemeris error %d\n", err_val);*/
    }
    get_satellite_ra_dec_delta( observer_loc, pos, &ra, &dec, &dist_to_satellite);
    epoch_of_date_to_j2000( time_jd, &ra, &dec);
    double az = 0;
    double elev = 0;
    ra_dec_to_az_alt(time_jd, DEG(obs.lat), DEG(obs.lon), DEG(ra)/15, DEG(dec), &az, &elev);
    /*printf("POS: %.4f,%.4f,%.4f\n", pos[0], pos[1], pos[2] );*/
    /*printf("VEL: %.4f,%.4f,%.4f\n", vel[0], vel[1], vel[2] );*/
    sat_pos_t ret;
    ret.az = az;
    ret.elev = elev;
    ret.ra = DEG(ra)/15;
    ret.dec = DEG(dec);
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
    double coarse_interval = 60.0 / 86400; //60 seconds in decimal days
    double fine_interval = 1.0 / 86400; //1 second in decimal days
    //search for when a pass comes over the horizon in larger steps
    while( jd < start_jd + search_time_days ) { //search next N day(s)
        jd += coarse_interval; 
        sat_pos_t s = calcSat( tle, jd, observer);
        aboveHorizonAtJD = s.elev >= 0;  
        i++;
        if( aboveHorizonAtJD ) {
            //we found a pass!
            break;
        }
    }
    //and since we found a pass, let's go figure out exactly where it is (in a super naive way)
    while( aboveHorizonAtJD ) {
        sat_pos_t s = calcSat( tle, jd, observer);
        aboveHorizonAtJD = s.elev >= 0;
        i++;
        //could speed this up by bisect bracketing the transition from - to +
        //should also find the end of the pass going + to -
        jd -= fine_interval; 
    }
    //printf("found pass at jd %f\n", jd);
    return jd;
}

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
    char * line1 = "1 25544U 98067A   21238.16818934  .00001738  00000-0  40314-4 0  9991";
    char * line2 = "2 25544  51.6461 356.6596 0002738 315.3214 107.2420 15.48517921299441";

    parse_elements( line1, line2, &iss_tle );
    sat_pos_t empty1 = {0};
    sat_pass_t empty2 = {0};
    sat_mem_t  iss = {"ISS", {1,0,0,0, 0,0,0,0}, iss_tle, empty1, empty2 };
    memcpy(&satellites[0], &iss, sizeof(sat_mem_t));
}
sat_mem_t satellites[5] = {0};
int num_satellites = 1;
const star_t stars[] = {
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
//#include "data/stars/stars.table" //~3150 more stars 
    //(not individually named, constellation abbreviations for the names)
//ahahaha yeah no, uses way too much space
//and WAY too long to draw, damn!
};
int num_stars = sizeof( stars ) / sizeof( star_t );

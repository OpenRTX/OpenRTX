#include <interfaces/rtc.h>
#include <satellite.h>
#include <stdio.h>
#include <math.h>

int close_enough(double a, double b, double plusminus){
	fprintf(stderr,"|%f - %f| = %f +- %f ", a, b, fabs(a-b), plusminus);
	fprintf(stderr,"%d ", fabs(a-b) < plusminus);
	fprintf(stderr,fabs(a-b) < plusminus ? "PASS\n": "FAIL\n");
	return fabs(a-b) < plusminus;
}

int test_local_sidereal_time(){
	const char * fn_name = "test_local_sidereal_time";
	int errors = 0;




	//using data from http://www.stargazing.net/kepler/altaz.html for a sanity check:
	//
	//Find the local siderial time for 2310 UT, 10th August 1998
	//at Birmingham UK (longitude 1 degree 55 minutes west).
	double jd = 2451036.46528;
	double lon = -1 * (1 + 55.0/60);
	//jd -> j2k
	double j2k = jd_to_j2k(jd);
	//j2k, longitude -> degrees
	double lst = local_sidereal_degrees(j2k, lon);
	//ret degrees
	double expected = 304.80762;


	if( !close_enough( lst, expected, .001) ){
		fprintf(stderr, "Failed %s in %s in line %d\n", fn_name, __FILE__,__LINE__);
		fprintf(stderr, "Input: jd %f, j2k %f, lon %f\n",jd, j2k, lon);
		fprintf(stderr, "expected %f, got %f\n",expected, lst);
		errors++;
	}

	return errors;
}
int test_hour_angle(){
	const char * fn_name = "test_hour_angle";
	int errors = 0;


	//using data from http://www.stargazing.net/kepler/altaz.html for a sanity check:
	//
	double ra_hrs = 16.695; //hours! 
	double lst = 304.80762; //degrees
	double expected = 54.382617;
	double ha = hour_angle_degrees( lst, ra_hrs);

	if( !close_enough( ha, expected, .001) ){
		fprintf(stderr, "Failed %s in %s in line %d\n", fn_name, __FILE__,__LINE__);
		fprintf(stderr, "Input: ra %f, lst %f\n", ra_hrs, lst);
		fprintf(stderr, "expected %f, got %f\n",expected, ha);
		errors++;
	}

	return errors;
}
int test_radec_to_azalt(){
	const char * fn_name = "test_radec_to_azalt";
	int errors = 0;

	//using data at bottom of http://www.stargazing.net/kepler/altaz.html for a sanity check:
	double jd = 2450522.29167; //14th March 1997 for Birmingham UK at 19:00 UT.
	//RA 22h 59.8min  DEC 42d 43min (epoch 1950, BAA comet section)
	//Days 73
	//Hours 1900
	//Long 1d 55min West Lat 52d 30min North
	double longitude = -1* (1 + 55.0/60);
	double latitude = (52 + 30.0/60);
	double ra = 22 + 59.8/60; //hours
	double dec = 42 + 43.0/60; //degrees
	//
	//expected:
	//LST = 6.367592 hrs
	//ALT = 22.40100 d
	//AZ  = 311.92258 d
	double az;
	double alt;
	double az_exp = 311.92258;
	double alt_exp = 22.401;
	ra_dec_to_az_alt( jd, latitude, longitude, ra, dec, &az, &alt);
	if( !close_enough( az, az_exp, .001) || ! close_enough( alt, alt_exp, .001) ){
		fprintf(stderr, "Failed %s in %s in line %d\n", fn_name, __FILE__,__LINE__);
		fprintf(stderr, "Input: jd %f lat %f lon %f ra %f dec %f\n", jd, latitude, longitude, ra, dec);
		fprintf(stderr, "az expected %f, got %f\n",az_exp, az);
		fprintf(stderr, "alt expected %f, got %f\n",alt_exp, alt);
		errors++;
	}
	printf("Expected az = 311.92258 d, alt = 22.401 d\n");
	printf("Got az = %f d, alt = %f d\n", az, alt);


	return errors;
}
int test_nextpass(){
	const char * fn_name = "test_nextpass";
	int errors = 0;


	//with this _specific_ ISS tle:
	char * line1 = "1 25544U 98067A   21240.05954104  .00002149  00000-0  47884-4 0  9998";
	char * line2 = "2 25544  51.6458 347.3122 0002703 322.9106 210.2991 15.48528782299732";
	tle_t tle;
	parse_elements( line1, line2, &tle);

	//ISS 2021 08 28
	//2021 08 28 13:28:55 local (UTC) it crosses the horizon up
	//pass starts at 2459455.06175
	double jd_expected = 2459455.06175;
	//and az 307.83
	double az_expected = 307.83;
	//
	//search start at 12:30 (after last pass has ended but before this one)
	double jd_start = 2459455.02083;
	topo_pos_t obs = {41.6726, -70.4629, 0};
	double nextpass = sat_nextpass( tle, jd_start, .1, obs);
	sat_pos_t start = calcSat( tle, nextpass, obs);
	fprintf(stderr,"azel %f %f radec %f %f \n", start.az, start.elev, start.ra, start.dec);

	double one_second = 1.0/86400; //jd is decimal days, so 1 second is just the inverse of number of seconds in a day
	if( !close_enough(nextpass,  jd_expected, one_second) || !close_enough( start.az, az_expected, 1) ){
		fprintf(stderr, "Failed %s in %s in line %d\n", fn_name, __FILE__,__LINE__);
		fprintf(stderr, "Input: custom ISS tle check 1\n");
		fprintf(stderr, "pass start off by %f seconds\n", 24*60*60*(nextpass - jd_expected));
		fprintf(stderr, "pass az off by %f degrees\n", (start.az - az_expected));
		errors++;
	}


	return errors;
}
int test_curtime_to_jd(){
	const char * fn_name = "test_curtime_to_jd";
	int errors = 0;
	curTime_t times[] = {
	//hour minute second day date month year
	//satellite stuff ignores "day" which is day of the week (sunday, monday, etc)
		{0,0,0,0,28,2,99},
		{6,57,44,0,28,8,21}, //Sat Aug 28 06:57:44 2021
		{12,0,0,0,1,1,0x00}, //J2000 epoch
		{12,0,0,0,1,1,0x00}, //J2000 epoch
		{12,0,0,0,1,1,0x00}, //J2000 epoch
	};
	double expected_jds[] = {
		//can use https://www.aavso.org/jd-calculator as a check
		2487762.500000, // NOT 2451237.500000 which is 1999, but 2099!
		2459454.7901,
		2451545.0,
		2451545.0 + 0.9/86400,
		2451545.0 + 0.99/86400,
	};
	int num_times = sizeof(expected_jds)/sizeof(double);
	for( int i = 0; i < num_times; i++){
		curTime_t t = times[i];
		double expected_jd = expected_jds[i];
		double jd = curTime_to_julian_day( t );
		double one_second = 1.0/86400; //jd is decimal days, so 1 second is just the inverse of number of seconds in a day
		//test to within one second - meaning there's a full two-second window (+1 and -1) of the target where any value in that range is okay
		//this is good enough for our purposes for now
		if( ! close_enough(jd, expected_jd, one_second) ){ 
			errors++;
			fprintf(stderr, "Failed %s in %s in line %d\n", fn_name, __FILE__,__LINE__);
			fprintf(stderr, "Input: %d:%d:%d %d/%d/%d\n",t.hour, t.minute, t.second, t.year, t.month, t.date);
			fprintf(stderr, "expected %f, got %f\n",expected_jd,jd);
		} else {
			//fprintf(stderr, "Passed %s in %s in line %d\n", fn_name, __FILE__,__LINE__);
			//fprintf(stderr, "Input: %d:%d:%d %d/%d/%d\n",t.hour, t.minute, t.second, t.year, t.month, t.date);
			//fprintf(stderr, "expected %f, got %f\n",expected_jd,jd);
		}
	}
	return errors;
}
int main(void){
	int errors = 0;
	errors += test_curtime_to_jd();
	errors += test_local_sidereal_time();
	errors += test_hour_angle();
	errors += test_radec_to_azalt();
	errors += test_nextpass();
	return errors;
}

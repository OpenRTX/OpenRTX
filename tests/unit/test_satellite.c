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
	double ra = 22 + 59.8/60;
	double dec = 42 + 43.0/60;
	//
	//expected:
	//LST = 6.367592 hrs
	//ALT = 22.40100 d
	//AZ  = 311.92258 d
	double az;
	double alt;
	ra_dec_to_az_alt( jd, latitude, longitude, ra, dec, &az, &alt);
	printf("Expected az = 311.92258 d, alt = 22.401 d\n");
	printf("Got az = %f d, alt = %f d\n", DEG(az), DEG(alt));



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
	errors += test_radec_to_azalt();
	return errors;
}

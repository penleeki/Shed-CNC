// Code for controlling a CNC machine (see https://hackaday.io/project/7603-garden-shed-cnc)
// written by Laurence Shann (laurence.shann@gmail.com)
//----------------------------------------------------------------------------------------
// This work is licensed under the Creative Commons Attribution 4.0 International License. 
// To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or 
// send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, 
// California, 94041, USA.
//----------------------------------------------------------------------------------------

#include "cnclib.h"
#include <limits.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

int cnclib::DECIMAL_PLACES = 2;
int cnclib::UNUSED_AXIS = INT_MAX;

// should probably replace these macros with proper standard functions
#define sqr(a) ((a)*(a))
#define round(a) ((a>=0)?floor((float)a+0.5):ceil((float)a-0.5))
#define max( a, b ) ( ( a > b) ? a : b ) 


vec3 cnclib::curve_delta(lineCoords coords, vec3 currentPosition){
	
	int rx = currentPosition.x-coords.arc.x;
	int ry = currentPosition.y-coords.arc.y;
	int64_t r2 = sqr(coords.to.x-coords.arc.x)+sqr(coords.to.y-coords.arc.y);
	float start_delta_angle = atan2((float)ry,(float)rx) + coords.arcDirection * M_PI/2;
	
	vec3 de;
	de.x = (int) round((float) -cos(start_delta_angle));
	de.y = (int) round((float) -sin(start_delta_angle));
	de.z = 0;
	
	int64_t sdist = cnclib::i_abs(sqr(rx+de.x)+ sqr(ry+de.y) - r2);
	
	if(de.x==0){
		int64_t sdistA = cnclib::i_abs(sqr(rx+1) + sqr(ry+de.y) - r2);
		int64_t sdistB = cnclib::i_abs(sqr(rx-1) + sqr(ry+de.y) - r2);
		if(sdistA<sdist && sdistA<sdistB) de.x=1;
		else if(sdistB<sdist && sdistB<sdistA) de.x=-1;
	}else if (de.y==0){
		int64_t sdistA = cnclib::i_abs(sqr(rx+de.x) + sqr(ry+1) - r2);
		int64_t sdistB = cnclib::i_abs(sqr(rx+de.x) + sqr(ry-1) - r2);
		if(sdistA<sdist && sdistA<sdistB) de.y=1;
		else if(sdistB<sdist && sdistB<sdistA) de.y=-1;
	}else{
		int64_t sdistA = cnclib::i_abs(sqr(rx+0) + sqr(ry+de.y) - r2);
		int64_t sdistB = cnclib::i_abs(sqr(rx+de.x) + sqr(ry+0) - r2);
		if(sdistA<sdist && sdistA<sdistB) de.x=0;
		else if(sdistB<sdist && sdistB<sdistA) de.y=0;
	}
	
	return de;
}

int cnclib::curve_numSteps(lineCoords coords){
	int len = 0;
	vec3 current;
	current.x = coords.from.x;
	current.y = coords.from.y;
	current.z = coords.from.z;
	do{
		vec3 d = curve_delta(coords, current);
		current.x+=d.x;
		current.y+=d.y;
		current.z+=d.z;
		len++;
	}while(current.x!=coords.to.x || current.y!=coords.to.y);
	return len;
}

vec3 cnclib::line_delta(lineCoords coords, int step, int totalSteps){
	vec3 r;
	r.x = line_axis(coords.to.x-coords.from.x, step, totalSteps);
	r.y = line_axis(coords.to.y-coords.from.y, step, totalSteps);
	r.z = line_axis(coords.to.z-coords.from.z, step, totalSteps);
	return r;
}

int cnclib::line_numSteps(lineCoords coords){
	int dx = coords.to.x - coords.from.x;
	int dy = coords.to.y - coords.from.y;
	int dz = coords.to.z - coords.from.z;
	if(dx<0) dx=-dx;
	if(dy<0) dy=-dy;
	if(dz<0) dz=-dz;
	return max(dx,max(dy,dz));
}

vec3 cnclib::addVec3(vec3 a, vec3 b){
	vec3 r;
	r.x = a.x+b.x;
	r.y = a.y+b.y;
	r.z = a.z+b.z;
	return r;
}

int cnclib::i_abs(int a){
	if(a>=0) return a;
	else return -a;
}

int cnclib::line_axis(int delta, int step, int totalSteps){
	int flipped=1;
	if(delta<0){ delta = -delta; flipped=-1; }
	int d = floor(0.5f + (float) delta * ((float)step/(float)totalSteps)) - floor(0.5f + (float) delta * ((float)(step-1)/(float)totalSteps));
	return d*flipped;
}

int cnclib::quick_pow10(int n)
{
    static int pow10[10] = {
        1, 10, 100, 1000, 10000, 
        100000, 1000000, 10000000, 100000000, 1000000000
    };
    return pow10[n]; 
}

int cnclib::valueFromStr(char* str, int ticksPermm)
{
	double f = strtod(str, NULL);
	f *= (double) ticksPermm;
	return round(f);
}

int cnclib::findNextKeyLetter(char* str, int offset)
{
	int i=offset;
	int r=-1;
	while(str[i] != '\0'){
		if(str[i] =='G'){r=i; break;}
		if(str[i] =='M'){r=i; break;}
		if(str[i] =='X'){r=i; break;}
		if(str[i] =='Y'){r=i; break;}
		if(str[i] =='Z'){r=i; break;}
		if(str[i] =='I'){r=i; break;}
		if(str[i] =='J'){r=i; break;}
        if(str[i] =='K'){r=i; break;}
        if(i > 10000){r=-1; break;}
		i++;
	}
	return r;
}

gCode cnclib::codeFromLine(char* str, int ticksPermm)
{
	gCode g;
	g.num = -1;
	g.x=g.y=g.z=g.i=g.j=g.k=UNUSED_AXIS;

	int command = findNextKeyLetter(str,0);
	
	while(command>=0){
		int v = valueFromStr(str+command+1, ticksPermm);
		switch(str[command]){
			case 'G':
				g.num = valueFromStr(str+command+1, 1);
				break;
			case 'X':
				g.x = v;
				break;
			case 'Y':
				g.y = v;
				break;
			case 'Z':
				g.z = v;
				break;
			case 'I':
				g.i = v;
				break;
			case 'J':
				g.j = v;
				break;
			case 'K':
				g.k = v;
				break;
			default:
				break;
		}

		command = findNextKeyLetter(str, command+1);
	}

	return g;
}

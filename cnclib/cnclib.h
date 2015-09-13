#pragma once

struct vec3
{
	int x,y,z;
};
struct gCode
{
	int num;
	int x,y,z;
	int i,j,k;
};

struct lineCoords{
	vec3 from;
	vec3 to;
	vec3 arc;
	int arcDirection;
};


class cnclib
{
public:
	static int DECIMAL_PLACES;
	static int UNUSED_AXIS;
	static gCode codeFromLine(char* str, int ticksPermm);
	static int line_numSteps(lineCoords coords);
	static vec3 line_delta(lineCoords coords, int step, int totalSteps);
	static int curve_numSteps(lineCoords coords);
	static vec3 curve_delta(lineCoords coords, vec3 currentPosition);
	static vec3 addVec3(vec3 a, vec3 b);
private:
	static int line_axis(int delta, int step, int totalSteps);
	static int quick_pow10(int n);
	static int valueFromStr(char* str, int ticksPermm);
	static int findNextKeyLetter(char* str, int offset);
	static int i_abs(int a);
};


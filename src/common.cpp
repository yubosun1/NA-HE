#include "common.h"

void coord2Int(Point p,int m, MappedPoint *mPoint)
{
	int nDims = 2, nBits = 4;

	unsigned int x = 0;
	unsigned int y = 0;
	double x_range[2] = { -180,180 };
	double y_range[2] = { -90,90 };
	//bitmask_t r;
	for (int i = m - 1; i >= 0; --i) {
		double y_mid = (y_range[0] + y_range[1]) / 2.;
		//std::cout << x << ',' << y <<std::endl;
		if (p.lat >= y_mid) {
			y |= 1 << i;
			y_range[0] = y_mid;
		}
		else {
			y_range[1] = y_mid;
		}
		double x_mid = (x_range[0] + x_range[1]) / 2.;
		if (p.lon >= x_mid) {
			x |= 1 << i;
			x_range[0] = x_mid;
		}
		else {
			x_range[1] = x_mid;
		}
	}
	mPoint->lon = y;
	mPoint->lat = x;
}
void point2Mappedpoint(Point p, MappedPoint *mPoint) {
	mPoint->lon = p.lon;
	mPoint->lat = p.lat;
}
vector<Point> readData(char *file)
{
	//FILE *fp = fopen(@"F:\jly\data\spatial\newyork","r");
	vector<Point> vec;
	std::ifstream fin(file, std::ios::in);
	char line[1024] = { 0 };
	std::string str_x = "", str_y="";
	double x, y;
	int m = 4;
	int count = 0;
	while (fin.getline(line, sizeof(line)))
	{
		std::stringstream word(line);
		word >> str_x;
		word >> str_y;
		x = std::stod(str_x);
		y = std::stod(str_y);
		Point p(x,y);
		//p.lon = x;
		//p.lat = y;
		vec.push_back(p);

	}
	fin.close();
	return vec;
}
#pragma  once
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <set>
using namespace std;
typedef unsigned long long bitmask_t;
typedef unsigned long halfmask_t;
enum Rotation {Up, Right, Down, Left};
//#define getBit1(X,i,k) (X>>(i-k+1)& ones(halfmask_t,k))
#define getBit(X,i) (X>>i & 1UL)
#define getBit1(X,i) (X>>(i)*2 & 3UL) 
#define getBit(X,i,k) (X>>(i-k+1)& ones(halfmask_t,k))  
//#define getBit1(X,i,y)y=((X>>i) & 1UL)
#define MAX(x,y) (x>y?x:y)

#define ones(T,k) ((((T)2) << (k-1)) - 1)

#define rdbit(w,k) (((w) >> (k)) & 1)

#define rotateRight(arg, nRots, nDims)                                  \
	((((arg) >> (nRots)) | ((arg) << ((nDims)-(nRots)))) & ones(bitmask_t,nDims))

#define rotateLeft(arg, nRots, nDims)                                   \
	((((arg) << (nRots)) | ((arg) >> ((nDims)-(nRots)))) & ones(bitmask_t,nDims))

#define adjust_rotation(rotation,nDims,bits)                            \
	do {                                                                    \
	/* rotation = (rotation + 1 + ffs(bits)) % nDims; */              \
	/*本操作相当于对bits的后n-1位取反末位加1*/  \
	bits &= -(signed)bits & nd1Ones;                                          \
	while (bits)                                                      \
	bits >>= 1, ++rotation;                                         \
	if ( ++rotation >= nDims )                                        \
	rotation -= nDims;                                              \
	} while (0)
 
//#define DLOGB_BIT_TRANSPOSE
static bitmask_t
	bitTranspose(unsigned nDims, unsigned nBits, bitmask_t inCoords)
#if defined(DLOGB_BIT_TRANSPOSE)
{
	unsigned const nDims1 = nDims - 1;
	unsigned inB = nBits;
	unsigned utB;
	bitmask_t inFieldEnds = 1;
	bitmask_t inMask = ones(bitmask_t, inB);
	bitmask_t coords = 0;

	while ((utB = inB / 2))
	{
		unsigned const shiftAmt = nDims1 * utB;
		bitmask_t const utFieldEnds =
			inFieldEnds | (inFieldEnds << (shiftAmt + utB));
		bitmask_t const utMask =
			(utFieldEnds << utB) - utFieldEnds;
		bitmask_t utCoords = 0;
		unsigned d;
		if (inB & 1)
		{
			bitmask_t const inFieldStarts = inFieldEnds << (inB - 1);
			unsigned oddShift = 2 * shiftAmt;
			for (d = 0; d < nDims; ++d)
			{
				bitmask_t in = inCoords & inMask;
				inCoords >>= inB;
				coords |= (in & inFieldStarts) << oddShift++;
				in &= ~inFieldStarts;
				in = (in | (in << shiftAmt)) & utMask;
				utCoords |= in << (d*utB);
			}
		}
		else
		{
			for (d = 0; d < nDims; ++d)
			{
				bitmask_t in = inCoords & inMask;
				inCoords >>= inB;
				in = (in | (in << shiftAmt)) & utMask;
				utCoords |= in << (d*utB);
			}
		}
		inCoords = utCoords;
		inB = utB;
		inFieldEnds = utFieldEnds;
		inMask = utMask;
	}
	coords |= inCoords;
	return coords;
}
#else
{
	bitmask_t coords = 0;
	unsigned d;
	for (d = 0; d < nDims; ++d)
	{
		unsigned b;
		bitmask_t in = inCoords & ones(bitmask_t, nBits);
		bitmask_t out = 0;
		inCoords >>= nBits;
		for (b = nBits; b--;)
		{
			out <<= nDims;
			out |= rdbit(in, b);
		}
		coords |= out << d;
	}
	return coords;
}
#endif


class Point
{ 
public:
	double lon;
	double lat; 
	Point(double lo, double la)
	{
		lon = lo;
		lat = la;
	}
};

class MappedPoint
{
public:
	halfmask_t lon;
	halfmask_t lat;
	
	/*bitmask_t lon;
	bitmask_t lat;*/
	bitmask_t index;
	MappedPoint();
	MappedPoint(halfmask_t lo, halfmask_t la)
	{
		lon = lo;
		lat = la;
	};
	MappedPoint(halfmask_t lo, halfmask_t la,bitmask_t ind)
	{
		lon = lo;
		lat = la;
		index = ind;
	};
	bool operator< (const MappedPoint &i) const {return index < i.index;}
};
class MappedPoint1
{
public:
	bitmask_t lon;
	bitmask_t lat;
	/*bitmask_t lon;
	bitmask_t lat;*/
	bitmask_t index;
	MappedPoint1();
	MappedPoint1(bitmask_t lo, bitmask_t la)
	{
		lon = lo;
		lat = la;
	};
	MappedPoint1(bitmask_t lo, bitmask_t la, bitmask_t ind)
	{
		lon = lo;
		lat = la;
		index = ind;
	};
	bool operator< (const MappedPoint1 &i) const { return index < i.index; }
};
void coord2Int(Point p,int m, MappedPoint *mPoint);
void point2Mappedpoint(Point p, MappedPoint *mPoint);
vector<Point> readData(char *file);
#include <stdio.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <gdiplus.h>
#include <math.h>
#include "common.h"
#include "neighbourAware.h"
#include "moore.h"
#include "burkardt.h"
#include "qhc.h"
#include "uniState.h"
#pragma comment(lib, "gdiplus.lib")
  
using namespace std;
using namespace Gdiplus;

#define LENG 256    //初始分段长度
#define PICNUMS 49 //图片的数量
#define XSIZE 512   //图像大小
#define YSIZE 512   //图像大小
#define NMAX 64    //分段最大长度
#define TH1 2000  //方差阈值
#define TH2 10  //合并小分段时对应的第二个分区的方差上界
#define TH3 10  //合并小分段时对应的第二个分区的长度上界
#define A  8  //压缩后的c_i的长度
#define M 49  // 图片数量
#define DBL_MAX 1.79769313486231570E+308
//void computeErrAndMean(char *data, )
//{
//
//}
typedef unsigned char uchar_t;
typedef unsigned int uint_t;
typedef unsigned long long ull_t;
//vector<U_64> lens;
//vector<U_64> grays;
struct CompressedImage
{
	int segNum;  //Segment Number
	vector<ull_t> segLenVec; //存放各分段长度
	vector<ull_t> segGrayVec;//存放各分段平均灰度
} Results[M];
enum hilbType{
	RAW, //raster scan
	UNISTATE,
	MOORE,
	BURKARDT,
	NEIGHBOUR};
int PicID = 0;
//int SegNo=0;
int BitsForSeg= log((double)NMAX)/log(2.0);

wchar_t* charToWChar(char* buffer)
{
	int iSize; 
	wchar_t* pwszUnicode ; 

	//返回接受字符串所需缓冲区的大小，已经包含字符结尾符'\0'
	iSize = MultiByteToWideChar(CP_ACP, 0, buffer , -1, NULL, 0); //iSize =wcslen(pwsUnicode)+1=6
	pwszUnicode = (wchar_t *)malloc(iSize*sizeof(wchar_t)); //不需要 pwszUnicode = (wchar_t *)malloc((iSize+1)*sizeof(wchar_t))
	MultiByteToWideChar(CP_ACP, 0, buffer , -1, pwszUnicode , iSize);
	return pwszUnicode;
}
 
void readRaw(Bitmap* bmp, UINT height,UINT width,uchar_t *data)
{
	Color color;
	for (UINT y = 0; y < height; y++)
		for (UINT x = 0; x < width; x++) {
			bmp->GetPixel(x, y, &color); 
			int t = color.GetBlue();
			data[y*XSIZE + x] = color.GetBlue(); //灰度图的red和green读到时坐标数据？
		}
}
//仅针对512*512的图像，故对应的Hilbert阶为9
void readByHilbert(Bitmap* bmp, UINT height,UINT width,uchar_t *data,hilbType type)
{
	bitmask_t prevlat = 0;
	bitmask_t prevlon = 0;
	int iterStartPos = 9 - 1;
	for(int i=0;i<height*width;i++)
	{
		Color color;
		bitmask_t coord[2] = { 0,0 };
		halfmask_t hGridX = 0, hGridY = 0;
		bitmask_t bGridX = 0, bGridY = 0;
		switch (type)
		{
		
		case UNISTATE:
			de_ush(i, hGridX, hGridY, 9);
			bmp->GetPixel(hGridX, hGridY, &color);
			data[i] = color.GetBlue();
			break;
		case MOORE:
			de_bnr(2, 9, i, coord);  //9阶
			bmp->GetPixel(coord[1], coord[0], &color);
			data[i] = color.GetBlue();
			break;

		case BURKARDT:
			de_arith(i, &hGridX, &hGridY, 9);
			bmp->GetPixel(hGridX, hGridY, &color);
			data[i] = color.GetBlue();
			break;

		case NEIGHBOUR:
			bitmask_t index = i;
			bitmask_t next_index = i + 1; 
			
			/*printf(" -> %lld %lld - ", prevlat, prevlon);
			printf("%d %lld %lld - ", iterStartPos, bGridX, bGridY);
			printf("%lld %lld - ", index, next_index);
			printf("%d \n", 9);*/

			de_neibourAware(prevlon, prevlat, iterStartPos, bGridX, bGridY, index, next_index, 9);

			bmp->GetPixel(prevlon, prevlat, &color);
			data[i] = color.GetBlue();
			break;
		}
	}
}

void readImageToArr(char* infilename,uchar_t *data,hilbType type)
{
	GdiplusStartupInput gdiplusstartupinput;
	ULONG_PTR gdiplustoken;
	GdiplusStartup(&gdiplustoken, &gdiplusstartupinput, NULL);
	int iSize;
 	Bitmap* bmp = new Bitmap(charToWChar(infilename));
	UINT height = bmp->GetHeight();
	UINT width = bmp->GetWidth();
	//cout << "width " << width << ", height " << height << endl;
	
	//ofstream fout(outfilename.c_str());
	if(type==RAW)
		readRaw(bmp,height,width,data);
	else
		readByHilbert(bmp,height,width,data,type);
	delete bmp;
	GdiplusShutdown(gdiplustoken);

}
//data：存储图像的数组
//pos：当前分区在data中的起始位置
//e：当前分区的方差
//aver：当前分区的灰度均值
void PARTITION(uchar_t * data,int pos, int N,double e,double aver,uint_t sumXpow,int &optN1,double &e1,double &aver1,uint_t &optXpow,double &e2,double &aver2)
{
	int curSum=0, curXpow = 0, optSum =0;
	double minErr = DBL_MAX,optAver=0;
	double curAver =0, curErr =0;
	for(int i=0;i<N-1;i++)
	{
		int N1 = i+1;
		curSum += data[pos+i];
		curXpow += data[pos+i]*data[pos+i]; 
		curAver =(double)curSum/N1;

		curErr = sumXpow - N1 *curAver *curAver -
			(N* aver-N1 * curAver)*(N* aver-N1 * curAver) / (N-N1); 
		if(curErr<minErr)
		{
			minErr = curErr;			
			optN1 = N1;
			optSum = curSum;			
			optAver = curAver;
			optXpow = curXpow;
		}
	}	
	aver1=optAver;
	e1 = optXpow - optN1 * optAver * optAver;
	aver2 =(aver * N -optSum)/(N-optN1);
	e2 = minErr - e1;
}

//val 指要append到vec的值，分别为长度或灰度。
//size 指该长度或灰度对应的二进制位长
void appendToVec(vector<ull_t> &vec,uchar_t val,int size)
{ 
	//vector内不足以存下一个val
	if((Results[PicID].segNum+1) * size >vec.size()*sizeof(ull_t))
	{
		vec.push_back(0ULL);
	}
	int currentTotalSize = Results[PicID].segNum * size;
	int wordID = currentTotalSize / (sizeof(ull_t)*8);
	int posInWord = currentTotalSize % (sizeof(ull_t)*8);
	int startOfNextWord = (wordID+1) * sizeof(ull_t)*8;
	if(currentTotalSize + size <= startOfNextWord)
		vec[wordID] = vec[wordID] | val << (sizeof(ull_t)*8-posInWord-size);
	else
	{
		int lensOfSecondPart = size +currentTotalSize - startOfNextWord;
		vec[wordID] = vec[wordID] | val>>lensOfSecondPart;
		vec[wordID+1] = vec[wordID+1] | (val & ones(uchar_t, lensOfSecondPart));
	}
	Results[PicID].segNum++;
}
//pos：第pos个分段
void GROUPLING(uchar_t * data,int pos,int N,double e, double aver, uint_t sumXpow)
{
	int N1 =0;
	uint_t sumX1pow =0;
	double e1 = 0,aver1 = 0,e2 = 0,aver2 = 0;
	if(N<=1 || N<=NMAX && e<=TH1)
	{
		appendToVec(Results[PicID].segLenVec,N,BitsForSeg);
		appendToVec(Results[PicID].segGrayVec,aver,BitsForSeg);
		return;
	} 	
	PARTITION(data,pos,N,e,aver,sumXpow,N1,e1,aver1,sumX1pow,e2,aver2);
	GROUPLING(data,pos,N1,e1,aver1,sumX1pow);
	GROUPLING(data,pos+N1,N-N1,e2,aver2,sumXpow-sumX1pow);
		 
}
//仅针对512*512的图像编写，对应的hilbert编码为9阶
/*void adjustByNeighbour(uchar_t *data)
{
	for(int i=0;i<XSIZE*YSIZE;i++)
	{
		halfmask_t GridX=0, GridY=0;
		de_neibourAware(i,GridX,GridY);  //此处我仅给了一个原型，请替换为自己实际的算法

	}
}
*/

void main()
{
	int i,j,k;
	int p=0;
	//RAW UNISTATE MOORE BURKARDT NEIGHBOUR

	//改变不同的类型，即可根据不同hilbert解码来测试压缩效率
	hilbType type = NEIGHBOUR;

	uchar_t *data= (uchar_t *)malloc(XSIZE*YSIZE);
	char *dir = "XXXXXX\\image dataset\\";
	vector<char*> files = getFilesList(dir);	

	LARGE_INTEGER Frequency;//计数器频率   
	LARGE_INTEGER start_PerformanceCount;//起始计数器   
	LARGE_INTEGER end_PerformanceCount;//结束计数器
	double decode_time_per_image;	//编码时间   
	double decode_time = 0;			//编码时间累计
	double compress_time_per_image;	//压缩时间   
	double compress_time = 0;		//压缩时间累计
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&start_PerformanceCount);

	for(i=0;i<files.size();i++)
	{ 

		QueryPerformanceCounter(&start_PerformanceCount);

		readImageToArr(files[i],data,type);

		QueryPerformanceCounter(&end_PerformanceCount);
		decode_time_per_image = (end_PerformanceCount.QuadPart - start_PerformanceCount.QuadPart) / (double)Frequency.QuadPart;
		decode_time += decode_time_per_image;

		QueryPerformanceCounter(&start_PerformanceCount);

		//char data[262144] = {42,4,235,86,65,45,334,65,22,37,89,77,89,76,34,123,231,235};
		int numberOfSegs = 512*512/LENG;

		for(j=0;j<XSIZE*YSIZE;j+=LENG)
		{
			//if(j == 68608)
			//	p = 1;
			uint_t sumGray =0;
			double averGray =0, error=0;
			uint_t sumXpow = 0;

			for(k=0;k<LENG;k++)  //暂不考虑最后一段不为LENG的情形，假设各段均相等
			{
				sumGray += data[j + k];
				sumXpow += data[j + k] * data[j + k];
			}
			averGray =sumGray/LENG;
			//printf("%f\n",averGray);
			for(k=0;k<LENG;k++)
				error += (data[j + k]-averGray)*(data[j + k]-averGray);

			GROUPLING(data,j,LENG,error,averGray,sumXpow);
		}
		PicID++;

		QueryPerformanceCounter(&end_PerformanceCount);
		compress_time_per_image = (end_PerformanceCount.QuadPart - start_PerformanceCount.QuadPart) / (double)Frequency.QuadPart;
		compress_time += compress_time_per_image;


		//printf("image[%2d] decode: %fs ;compress: %fs.\n", i + 1, decode_time_per_image, compress_time_per_image);


	}

	printf("decode     => %fs.\n", decode_time);
	printf("compress   => %fs.\n", compress_time);


} 
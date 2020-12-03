#ifndef CONV_H_
#define CONV_H_
#include <string.h>

#define CHin 64
#define CHout 64
#define R 13
#define C 13
#define K 3
#define S 1

//
#define Tin 8
#define Tout 16
#define Tr 13
#define Tc 13


typedef int data_t;
typedef struct
{
	data_t in_data0;
	data_t in_data1;
	data_t in_data2;
	data_t in_data3;
}AXI_type;

enum layertype{
	conv
};


void test(AXI_type *In_addr,AXI_type *W_addr,data_t *Out_addr);

#endif

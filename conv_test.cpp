#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "conv.h"

#define CHin_a CHin
#define CHout_a CHout
#define R_a R
#define C_a C
#define K_a K
#define S_a S



int main()
{
	srand((unsigned)time(NULL));


	data_t in[CHin_a][S_a*R_a-S_a+K_a][S_a*C_a-S_a+K_a];
	data_t W[CHout_a][CHin_a][K_a][K_a];
	data_t out[CHout_a][R_a][C_a]={0};
	data_t out_s[CHout_a][R_a][C_a]={0};

	data_t *in_p=(data_t*)malloc((CHin_a)*(S_a*R_a-S_a+K_a)*(S_a*C_a-S_a+K_a)*sizeof(data_t));
	if(in_p==NULL)
	{
		printf("Error1");
		exit(0);
	}

	data_t *W_p=(data_t*)malloc((CHout_a)*(CHin_a)*(K_a)*(K_a)*sizeof(data_t));
	if(W_p==NULL)
	{
		printf("Error3");
		exit(0);
	}

	data_t *out_p=&out[0][0][0];

	AXI_type *in_a=(AXI_type *)in_p;
	AXI_type *W_a=(AXI_type *)W_p;

	data_t temp;


	// input
	for(int i=0;i<CHin_a;i++)
		for(int j=0;j<S_a*R_a-S_a+K_a;j++)
			for(int z=0;z<S_a*C_a-S_a+K_a;z++)
			{
				temp = rand()%10;
				in[i][j][z]=(i+j+z)%10;
			}

	// input数据重排后映射到FPGA上
	for(int j=0;j<S_a*R_a-S_a+K_a;j++)
		for(int z=0;z<S_a*C_a-S_a+K_a;z++)
			for(int i=0;i<CHin_a;i++)
			{
				temp=in[i][j][z];
				*((data_t*)in_p++)=temp;
			}

	// weight
	for(int i=0;i<CHout_a;i++)
		for(int j=0;j<CHin_a;j++)
			for(int z=0;z<K_a;z++)
				for(int d=0;d<K_a;d++)
				{
					temp=rand()%10;
					W[i][j][z][d]=(i+j+z+d)%10;
				}

	// weight数据重排后映射到FPGA上
	for(int z=0;z<K_a;z++)
		for(int d=0;d<K_a;d++)
			for(int i=0;i<CHout_a;i++)
				for(int j=0;j<CHin_a;j++)
				{
					temp=W[i][j][z][d];
					*((data_t*)W_p++)=temp;
				}

	test(in_a,W_a,out_p);

	free(in_p);  in_p=NULL;
	free(W_p);   W_p=NULL;

    // tb_convolution
	for(int kr=0;kr<K_a;kr++)
	{
		for(int kc=0;kc<K_a;kc++)
		{
			for(int r=0;r<R_a;r++)
			{
				for(int c=0;c<C_a;c++)
				{
					for(int out=0;out<CHout_a;out++)
					{
						for(int in_c=0;in_c<CHin_a;in_c++)
						{
							out_s[out][r][c]+=in[in_c][S_a*r+kr][S_a*c+kc]*W[out][in_c][kr][kc];
						}
					}
				}
			}
		}
	}


	// Comparison
	for(int i=0;i<CHout_a;i++)
		for(int j=0;j<R_a;j++)
			for(int z=0;z<C_a;z++)
			{
				//printf("out[%d][%d][%d]=%d\n",i,j,z,out[i][j][z]);
				// printf("out_s[%d][%d][%d]=%d\n",i,j,z,out_s[i][j][z]);
				// printf("out[%d][%d][%d]=%d\n",i,j,z,out[i][j][z]);
				if(out[i][j][z]!=out_s[i][j][z])   //
				{
					printf("out_s[%d][%d][%d]=%d\n",i,j,z,out_s[i][j][z]);
					printf("test fail!!!\n");
					return 1;
				}
			}

	for(int i=0;i<CHout_a;i++)
		for(int j=0;j<R_a;j++)
			for(int z=0;z<C_a;z++)
			{
				if(out[i][j][z]!=out_s[i][j][z])
				{
					printf("test fail!!!\n");
					return 1;
				}
			}
	printf("Test pass!!!\n");

	return 0;
}

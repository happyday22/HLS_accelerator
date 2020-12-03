#include "conv.h"

void Load_In(AXI_type *In_addr,data_t In[Tin][S*Tr+K-S][S*Tc+K-S],int Tin_c,int Tr_c,int Tc_c,bool exe)
{
	if(!exe)
		return;

	for(int L_ri=0;L_ri<S*Tr+K-S;L_ri++)
	{
		for(int L_ci=0;L_ci<S*Tc+K-S;L_ci++)
		{
			for(int L_chi=0;L_chi<Tin;L_chi+=4) // 暂时要求是4的倍数
			{
#pragma HLS PIPELINE
				int offset=(((L_ri+Tr_c)*(S*C+K-S)+(L_ci+Tc_c))*CHin+(L_chi+Tin_c))/4;
				if((L_chi+Tin_c)<CHin && (L_ri+Tr_c)<(S*R+K-S) && (L_ci+Tc_c)<(S*C+K-S))
				{
					In[L_chi][L_ri][L_ci] = ((AXI_type*)In_addr+offset)->in_data0;
					In[L_chi+1][L_ri][L_ci] = ((AXI_type*)In_addr+offset)->in_data1;
					In[L_chi+2][L_ri][L_ci] = ((AXI_type*)In_addr+offset)->in_data2;
					In[L_chi+3][L_ri][L_ci] = ((AXI_type*)In_addr+offset)->in_data3;
				}
				else
				{
					// 分片后缓存填充也要求是4的倍数
					In[L_chi][L_ri][L_ci] = (data_t)0;
					In[L_chi+1][L_ri][L_ci] = (data_t)0;
					In[L_chi+2][L_ri][L_ci] = (data_t)0;
					In[L_chi+3][L_ri][L_ci] = (data_t)0;
				}

			}
		}
	}
}


void Load_W(AXI_type *W_addr,data_t W[Tout][Tin][K][K],int Tin_c,int Tout_c,int exe)
{
	if(!exe)
		return;

	for(int L_kr=0;L_kr<K;L_kr++)
	{
		for(int L_kc=0;L_kc<K;L_kc++)
		{
			for(int L_cho=0;L_cho<Tout;L_cho++)
			{
				for(int L_chi=0;L_chi<Tin;L_chi+=4)  // 暂时要求是4的倍数
				{
#pragma HLS PIPELINE
					int offset=((((L_kr*K)+L_kc)*CHout+(L_cho+Tout_c))*CHin+(L_chi+Tin_c))/4;
					if((L_cho+Tout_c)<CHout && (L_chi+Tin_c)<CHin)
					{
						W[L_cho][L_chi][L_kr][L_kc]=((AXI_type*)W_addr+offset)->in_data0;
						W[L_cho][L_chi+1][L_kr][L_kc]=((AXI_type*)W_addr+offset)->in_data1;
						W[L_cho][L_chi+2][L_kr][L_kc]=((AXI_type*)W_addr+offset)->in_data2;
						W[L_cho][L_chi+3][L_kr][L_kc]=((AXI_type*)W_addr+offset)->in_data3;
					}
					else
					{
						W[L_cho][L_chi][L_kr][L_kc]=(data_t)0;
						W[L_cho][L_chi+1][L_kr][L_kc]=(data_t)0;
						W[L_cho][L_chi+2][L_kr][L_kc]=(data_t)0;
						W[L_cho][L_chi+3][L_kr][L_kc]=(data_t)0;
					}
				}
			}
		}
	}
}


void Conv(data_t In[Tin][S*Tr+K-S][S*Tc+K-S],data_t W[Tout][Tin][K][K],data_t Out[Tout][Tr][Tc],int exe)
{
	if(!exe)
		return;

	Kernel_Row:
	for(int kr=0;kr<K;kr++)
	{
		Kernel_Col:
		for(int kc=0;kc<K;kc++)
		{
			Row:
			for(int r=0;r<Tr;r++)
			{
				Column:
				for(int c=0;c<Tc;c++)
				{
#pragma HLS PIPELINE
					Out_channel:
					for(int out=0;out<Tout;out++)
					{
						in_channel:
						for(int in=0;in<Tin;in++)
						{
							Out[out][r][c]+=In[in][S*r+kr][S*c+kc]*W[out][in][kr][kc];
						}
					}
				}
			}
		}
	}



}

void Offload_out(data_t* out_addr,data_t Out[Tout][Tr][Tc],int Tout_c,int Tr_c,int Tc_c,int exe)
{
	if(!exe) return;
	for(int S_cho=0;S_cho<Tout;S_cho++)
	{
		for(int L_ro=0;L_ro<Tr;L_ro++)
		{
			for(int L_co=0;L_co<Tc;L_co++)
			{
#pragma HLS PIPELINE
				int offset= (((S_cho+Tout_c)*R)+(L_ro+Tr_c))*C + (L_co+Tc_c);
				if((S_cho+Tout_c)<CHout && (L_ro+Tr_c)<R && (L_co+Tc_c)<C)
					*(out_addr+offset) = Out[S_cho][L_ro][L_co];
				Out[S_cho][L_ro][L_co]=(data_t)0;    // clear buffer.
			}
		}
	}
}


void compute_out(AXI_type *In_addr,AXI_type *W_addr,int r,int c,int cho,data_t Out[Tout][Tr][Tc],int exe)
{
	if(!exe) return;

	data_t In[Tin][S*Tr+K-S][S*Tc+K-S];
#pragma HLS ARRAY_PARTITION variable=In complete dim=1
	data_t In1[Tin][S*Tr+K-S][S*Tc+K-S];
#pragma HLS ARRAY_PARTITION variable=In1 complete dim=1
	data_t W[Tout][Tin][K][K];
#pragma HLS ARRAY_PARTITION variable=W complete dim=1
#pragma HLS ARRAY_PARTITION variable=W complete dim=2
	data_t W1[Tout][Tin][K][K];
#pragma HLS ARRAY_PARTITION variable=W1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=W1 complete dim=2

	int pingpong=0;
	for(int chi=0;chi<CHin+Tin;chi+=Tin)   //no_pipe_ok
	{
		if(pingpong==0)
		{
			Load_In(In_addr,In1,chi,r,c,chi<CHin);
			Load_W(W_addr,W1,chi,cho,chi<CHin);
			Conv(In,W,Out,chi!=0);
			pingpong=1;
		}
		else
		{
			Load_In(In_addr,In,chi,r,c,chi<CHin);
			Load_W(W_addr,W,chi,cho,chi<CHin);
			Conv(In1,W1,Out,chi!=0);
			pingpong=0;
		}
	}
	return;
}


void Conv_deal(AXI_type *In_addr,AXI_type *W_addr,data_t *Out_addr)
{
	static data_t Out[Tout][Tr][Tc];
#pragma HLS ARRAY_PARTITION variable=Out complete dim=1
	static data_t Out_1[Tout][Tr][Tc];
#pragma HLS ARRAY_PARTITION variable=Out_1 complete dim=1

	int flag=0;

	r_channel_tiling:
	for(int r=0;r<R;r+=Tr)
	{
		c_channel_tiling:
		for(int c=0;c<C;c+=Tc)
		{
			out_channel_tiling:

			for(int cho=0;cho<CHout+Tout;cho+=Tout)
			{
				if(flag==0){
					compute_out(In_addr,W_addr,r,c,cho,Out_1,cho<CHout);
					Offload_out(Out_addr,Out,cho-Tout,r,c,cho!=0);
					flag=1-flag;

				}else if(flag==1)
				{
					compute_out(In_addr,W_addr,r,c,cho,Out,cho<CHout);
					Offload_out(Out_addr,Out_1,cho-Tout,r,c,cho!=0);
					flag=1-flag;
				}
			}
		}
	}
	return;
}


void test(AXI_type *In_addr,AXI_type *W_addr,data_t *Out_addr)
{
#pragma HLS DATA_PACK variable=W_addr
#pragma HLS DATA_PACK variable=In_addr

#pragma HLS INTERFACE m_axi depth=10800 port=In_addr bundle=BUS0
#pragma HLS INTERFACE m_axi depth=55296 port=W_addr bundle=BUS2
#pragma HLS INTERFACE m_axi depth=21632 port=Out_addr bundle=BUS3
#pragma HLS INTERFACE s_axilite port=return bundle=CTRL

	int type=0;
	if(type==conv)
		Conv_deal(In_addr,W_addr,Out_addr);

	return;
}


#include "predict.h"
#include <stdio.h>
#include<string.h>
#include<time.h>
#include<stdlib.h>

//基础数据
struct VPS_Type vps_type[15] = {0};//虚拟服务器规格列表
struct PS_Type ps_type = {0};//物理服务器规格

int vps_type_num = 0;//虚拟服务器规格的数量

char target[4] = {0};//优化目标

long predict_starttime = 0;//预测开始时间
long predict_endtime = 0;//预测结束时间

//历史数据
long history_time[MAX_DATA_NUM] = {0};//历史数据中的时间
int history[MAX_DATA_NUM][MAX_INFO_NUM] = {0};//历史数据

//预测数据
long future_time[14] = {0};//预测数据中的时间
int future[14][MAX_INFO_NUM] = {0};//预测数据

//汇总数据
int future_total[MAX_INFO_NUM] = {0};//全部的预测数据
int required_future_total[15] = {0};//需求的预测数据

//部署数据
int dispose[MAX_DATA_NUM][15] = {0};//部署数据
int vps_num = 0;//预测的虚拟服务器数量
int ps_num = 0;//需要的物理服务器数量

double cost_effective[15] = {0};//贪心性价比

//你要完成的功能总入口
void predict_server(char * info[MAX_INFO_NUM], char * data[MAX_DATA_NUM], int data_num, char * filename)
{
    get_info(info);
    //print_info();
    get_data(data, data_num);
    //print_data(data, data_num);
    //print_history();

    predict_data();
    //print_future_total();

    get_required_future_total();
    //print_get_required_future_total();

    arrange_vps();
    //print_dispose();

    // 需要输出的内容
    char result_file[100000] = "";
    char str_tmp[100] = "";
    sprintf(str_tmp,"%d\n",vps_num);
    strcat(result_file,str_tmp);
    for(int i = 0; i < vps_type_num; i ++)
    {
        sprintf(str_tmp,"flavor%d %d\n",vps_type[i].flavorID, future_total[vps_type[i].flavorID-1]);
        strcat(result_file,str_tmp);
    }
    strcat(result_file,"\n");
    sprintf(str_tmp,"%d\n",ps_num);
    strcat(result_file,str_tmp);
    for(int i = 0; i < ps_num; i ++)
    {
        sprintf(str_tmp,"%d",i + 1);
        strcat(result_file,str_tmp);
        for(int j = 0;j < 15;j ++)
        {
            if(dispose[i][j] != 0)
            {
                sprintf(str_tmp," flavor%d %d",vps_type[j].flavorID,dispose[i][j]);
                strcat(result_file,str_tmp);
            }
        }
        strcat(result_file,"\n");
    }
/*
    	// 需要输出的内容
    	char * result_file = (char *)"17\n\n0 8 0 20";
*/
    // 直接调用输出文件的方法输出到指定文件中(ps请注意格式的正确性，如果有解，第一行只有一个数据；第二行为空；第三行开始才是具体的数据，数据之间用一个空格分隔开)
    write_result(result_file, filename);
}

//字符串时间转换成时间戳格式，便于后面的计算
long metis_strptime(char *str_date, char *str_time)
{
    char timestamp[20] = "";
    strcat(timestamp,str_date);
    strcat(timestamp," ");
    strcat(timestamp,str_time);
    struct tm stm;
    strptime(timestamp, "%Y-%m-%d %H:%M:%S",&stm);
    long t = mktime(&stm);
    t = t + 8*60*60;//时区转换
    return t;
}

//获取info文件里的数据
void get_info(char * info[MAX_INFO_NUM])
{
    char predict_starttime_date[11] = {0};//预测开始日期
    char predict_starttime_time[11] = {0};//预测开始时间
    char predict_endtime_date[11] = {0};//预测结束日期
    char predict_endtime_time[11] = {0};//预测结束时间

    sscanf(info[0],"%d %d %d",&ps_type.cpu,&ps_type.mem,&ps_type.hd);
    //ps_type.mem = ps_type.mem * 1024;//将物理服务器的内存单位GB转换成MB统一单位
    sscanf(info[2],"%d",&vps_type_num);
    int i = 3;
    while(i <= 3 + vps_type_num)
    {
        sscanf(info[i],"flavor%d %d %d",&vps_type[i-3].flavorID,&vps_type[i-3].cpu,&vps_type[i-3].mem);
        vps_type[i-3].mem = vps_type[i-3].mem / 1024;//将虚拟服务器的内存单位MB转换成GB统一单位
        i ++;
    }
    sscanf(info[i],"%s",target);
    sscanf(info[i + 2],"%s%s",predict_starttime_date,predict_starttime_time);
    predict_starttime = metis_strptime(predict_starttime_date, predict_starttime_time);//将字符串时间转换成时间戳
    sscanf(info[i + 3],"%s%s",predict_endtime_date,predict_endtime_time);
    predict_endtime = metis_strptime(predict_endtime_date, predict_endtime_time);//将字符串时间转换成时间戳
}

void print_info()
{
    printf("PS:cpu:%d, mem:%d, hd:%d\n",ps_type.cpu,ps_type.mem,ps_type.hd);
    printf("VPS: %d\n",vps_type_num);
    for(int i = 0; i < vps_type_num; i ++)
    {
        printf("flavor%d %d %d\n",vps_type[i].flavorID,vps_type[i].cpu,vps_type[i].mem);
    }
    printf("%s\n",target);
    printf("%ld\n",predict_starttime);
    printf("%ld\n",predict_endtime);
}

//获取训练数据集里的数据
void get_data(char * data[MAX_DATA_NUM], int data_num)
{
    int flavorID = 0;
    char str_date[11] = {0};
    char str_time[11] = {0};
    long time = 0;
    int t = 0;

    for(int i = 0; i < data_num; i ++)
    {
        sscanf(data[i],"%*s flavor%d%s%s",&flavorID,str_date,str_time);
        time = metis_strptime(str_date, str_time);
        if(history_time[t] == 0)
        {
            history_time[t] = time - time % (60*60*24);
            history[t][flavorID-1] ++;
        }
        else if(time - history_time[t] < 60*60*24)
        {
            history[t][flavorID-1] ++;
        }
        else if (time - history_time[t] > 60*60*24)
        {
            t ++;
            history_time[t] = time - time % (60*60*24);
            history[t][flavorID-1] ++;
        }
    }
}

void print_data(char * data[MAX_DATA_NUM], int data_num)
{
    for(int i = 0; i < data_num; i ++)
    {
        printf("%s\n",data[i]);
    }
}

void print_history()
{
    for(int t = 0; history_time[t] != 0; t ++)
    {
        printf("%ld:",history_time[t]);
        for(int i = 0; i < MAX_INFO_NUM; i ++)
        {
            printf("%d ",history[t][i]);
        }
        printf("\n");
    }
}

//预测数据（汇总预测数据），后面预测和汇总要分开
void predict_data()
{
    int history_date_count = 0;
    for(history_date_count = 0; history_time[history_date_count] != 0; history_date_count ++);

    double predict_date_count = (predict_endtime-predict_starttime) / (60*60*24);
    double future_total_tmp[MAX_INFO_NUM] = {0};

    for(int i = 0; i < history_date_count; i ++)
    {
        for(int j = 0; j < MAX_INFO_NUM; j ++)
        {
            future_total_tmp[j] = future_total_tmp[j] + history[i][j];
        }
    }
    for(int j = 0; j < MAX_INFO_NUM; j ++)
    {
        future_total[j] = (int)(future_total_tmp[j] / history_date_count * predict_date_count);
    }
}

void print_future_total()
{
    for(int j = 0; j < MAX_INFO_NUM; j ++)
    {
        printf("flavor%d:%d\n", j +1, future_total[j]);
    }
}

//筛选题目需要的预测数据
void get_required_future_total()
{
    for(int i = 0; i < vps_type_num; i ++)
    {
        required_future_total[i] =  future_total[vps_type[i].flavorID-1];
        vps_num = vps_num + required_future_total[i];
    }
}

void print_get_required_future_total()
{
    for(int i = 0; i < vps_type_num; i ++)
    {
        printf("flavor%d %d %d %d\n",vps_type[i].flavorID,vps_type[i].cpu,vps_type[i].mem,required_future_total[i]);
    }
}

//部署虚拟服务器
void arrange_vps()
{
    /*for(int i = 0; i < vps_type_num; i ++)
    {
        for(int j = 0; j < required_future_total[i]; j ++)
        {
            dispose[ps_num][i] = 1;
            ps_num ++;
        }
    }*/
    for(int i = 0;i < vps_type_num;i ++)
    {
        if(strcmp(target, "CPU") == 0)
        {
            cost_effective[i] = (double)vps_type[i].cpu / (double)vps_type[i].mem;
        }
        else if(strcmp(target, "MEM") == 0)
        {
            cost_effective[i] = (double)vps_type[i].mem / (double)vps_type[i].cpu;
        }
    }

    for(int j = vps_num; j > 0; j --)
    {
        int k_max = 0;
        int sum_cpu = 0;
        int sum_mem = 0;
        for(;required_future_total[k_max] == 0;k_max ++);

        for(int i = k_max+1 ; i < vps_type_num; i ++)
        {
            if(cost_effective[k_max] < cost_effective[i] && required_future_total[i] != 0)
            {
                k_max = i;
            }
        }

        for(int i = 0; i < vps_type_num; i ++)
        {
            sum_cpu = sum_cpu + vps_type[i].cpu * dispose[ps_num][i];
            sum_mem = sum_mem + vps_type[i].mem * dispose[ps_num][i];
        }

        if((sum_cpu +  vps_type[k_max].cpu) <= ps_type.cpu && (sum_mem + vps_type[k_max].mem) <= ps_type.mem)
        {
            dispose[ps_num][k_max] = dispose[ps_num][k_max] + 1;
            required_future_total[k_max] --;
        }
        else
        {
            ps_num ++;
            dispose[ps_num][k_max] = dispose[ps_num][k_max] + 1;
            required_future_total[k_max] --;
        }
    }
    ps_num ++;
}

void print_dispose()
{
    printf("%d\n", ps_num);
    for(int i = 0; i < ps_num; i ++)
    {
        printf("%d:", i + 1);
        for(int j = 0; j < vps_type_num; j ++)
        {
            printf("flavor%d %d ", vps_type[j].flavorID, dispose[i][j]);
        }
        printf("\n");
    }
}
//dp_vmp
void arrangement()
{
	
	int utilization_rate;//利用率
	while(vps_type_num!0)
	{
		int sum_cpu = 0;
		int sum_mem = 0;
	    for(int i = 0; i < vps_type_num; i ++)
	    {
	        sum_cpu = sum_cpu + vps_type[i].cpu * dispose[ps_num][i];//疑问
	        sum_mem = sum_mem + vps_type[i].mem * dispose[ps_num][i];
	    }

	    //基于cpu和内存的需求程度
	    {
	    	n[l]=vps_type[i].cpu/ps_type.cpu+vps_type[i].mem/ps_type.mem;
	    	sort n[l];
	    }

	    if(sum_cpu<ps_type.cpu&&sum_cpu<ps_type.cpu)
	    {
	    	dispose[ps_num][i]=dispose[ps_num][i] + 1;
	    	delete[vps_num];
	    }
	    else
	    {
	    	sort n[l];
	    	n=vps_type_num;
	    	for(int j = 0;j<ps_type.cpu;j++)
	    		for (int m = 0;m<ps_type.mem;m++)
	    		{
	    			for(int i = 0;i<n;i++)
	    			{
	    				if(i==0||j==0||m==0)
	    				{
	    					//状态方程
	    					f_cpu[i][j][m]=0;
	    					f_mem[i][j][m]=0;
	    				}
	    				else
	    				{
	    					if (j<vps_type[i].cpu&&m<vps_type[i].mem)
	    					{
	    						f_cpu[i][j][m]=f_cpu[i-1][j][m];
	    						f_mem[i][j][m]=f_mem[i-1][j][m];
	    					}
	    					else
	    					{
	    						//计算w1,w2,u1,u2

	    						w1=sqrt((pow(1-utilization_rate[s][cpu]),2)+(pow(1-utilization_rate[s][mem]),2));
	    						w2=sqrt((pow(1-utilization_rate[s+1][cpu]),2)+(pow(1-utilization_rate[s+1][mem]),2));
	    						u1=sqrt((pow(utilization_rate[all][cpu]-utilization_rate[s][cpu]),2)+(pow(utilization_rate[all][mem]-utilization_rate[s][mem]),2));
	    						u1=sqrt((pow(utilization_rate[all][cpu]-utilization_rate[s+1][cpu]),2)+(pow(utilization_rate[all][mem]-utilization_rate[s+1][mem]),2));
	    						//作出放置第i+1台虚拟机的决策条件
	    						if(u2<u1||(-w2)*log(max(vps_type[i+1].mem,vps_type[i+1].cpu))<(-w1)*log(max(vps_type[i].mem,vps_type[i].cpu)))
	    						{
	    							f_cpu[i+1][j][m]=f_cpu[i][j-(i+1)][m-(i+1)]+(i+1);
	    						}
	    						else
	    						{
	    							f_cpu[i+1][j][m]=f_cpu[i][j][m];
	    						}
	    					}
	    				}
	    			}
	    		}
	    remain_cpu=ps_type.cpu-sum(f_cpu);
	    remain_mem=ps_type.mem-sum(f_mem);
	    for(i=n;i>1;i--)
	    {
	    	w1=sqrt((pow(1-utilization_rate[s][cpu]),2)+(pow(1-utilization_rate[s][mem]),2));
	    	w2=sqrt((pow(1-utilization_rate[s+1][cpu]),2)+(pow(1-utilization_rate[s+1][mem]),2));
	    	u1=sqrt((pow(utilization_rate[all][cpu]-utilization_rate[s][cpu]),2)+(pow(utilization_rate[all][mem]-utilization_rate[s][mem]),2));
	    	u1=sqrt((pow(utilization_rate[all][cpu]-utilization_rate[s+1][cpu]),2)+(pow(utilization_rate[all][mem]-utilization_rate[s+1][mem]),2));
	    	dispose[ps_num][i]=f_cpu[i][j][m];`		//疑问
	    	delete(vps_type_num[i]);
	    	remain_vps_mem=sum_mem-vps_type.mem;
	    	remain_vps_cpu=sum_cpu-vps_type.cpu;
	    }
	}
}














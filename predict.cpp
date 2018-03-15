#include "predict.h"
#include <stdio.h>
#include<string.h>
#include<time.h>
#include<stdlib.h>

//��������
struct VPS_Type vps_type[15] = {0};//�������������б�
struct PS_Type ps_type = {0};//������������

int vps_type_num = 0;//�����������������

char target[4] = {0};//�Ż�Ŀ��

long predict_starttime = 0;//Ԥ�⿪ʼʱ��
long predict_endtime = 0;//Ԥ�����ʱ��

//��ʷ����
long history_time[MAX_DATA_NUM] = {0};//��ʷ�����е�ʱ��
int history[MAX_DATA_NUM][MAX_INFO_NUM] = {0};//��ʷ����

//Ԥ������
long future_time[14] = {0};//Ԥ�������е�ʱ��
int future[14][MAX_INFO_NUM] = {0};//Ԥ������

//��������
int future_total[MAX_INFO_NUM] = {0};//ȫ����Ԥ������
int required_future_total[15] = {0};//�����Ԥ������

//��������
int dispose[MAX_DATA_NUM][15] = {0};//��������
int vps_num = 0;//Ԥ����������������
int ps_num = 0;//��Ҫ���������������

double cost_effective[15] = {0};//̰���Լ۱�

//��Ҫ��ɵĹ��������
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

    // ��Ҫ���������
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
    	// ��Ҫ���������
    	char * result_file = (char *)"17\n\n0 8 0 20";
*/
    // ֱ�ӵ�������ļ��ķ��������ָ���ļ���(ps��ע���ʽ����ȷ�ԣ�����н⣬��һ��ֻ��һ�����ݣ��ڶ���Ϊ�գ������п�ʼ���Ǿ�������ݣ�����֮����һ���ո�ָ���)
    write_result(result_file, filename);
}

//�ַ���ʱ��ת����ʱ�����ʽ�����ں���ļ���
long metis_strptime(char *str_date, char *str_time)
{
    char timestamp[20] = "";
    strcat(timestamp,str_date);
    strcat(timestamp," ");
    strcat(timestamp,str_time);
    struct tm stm;
    strptime(timestamp, "%Y-%m-%d %H:%M:%S",&stm);
    long t = mktime(&stm);
    t = t + 8*60*60;//ʱ��ת��
    return t;
}

//��ȡinfo�ļ��������
void get_info(char * info[MAX_INFO_NUM])
{
    char predict_starttime_date[11] = {0};//Ԥ�⿪ʼ����
    char predict_starttime_time[11] = {0};//Ԥ�⿪ʼʱ��
    char predict_endtime_date[11] = {0};//Ԥ���������
    char predict_endtime_time[11] = {0};//Ԥ�����ʱ��

    sscanf(info[0],"%d %d %d",&ps_type.cpu,&ps_type.mem,&ps_type.hd);
    //ps_type.mem = ps_type.mem * 1024;//��������������ڴ浥λGBת����MBͳһ��λ
    sscanf(info[2],"%d",&vps_type_num);
    int i = 3;
    while(i <= 3 + vps_type_num)
    {
        sscanf(info[i],"flavor%d %d %d",&vps_type[i-3].flavorID,&vps_type[i-3].cpu,&vps_type[i-3].mem);
        vps_type[i-3].mem = vps_type[i-3].mem / 1024;//��������������ڴ浥λMBת����GBͳһ��λ
        i ++;
    }
    sscanf(info[i],"%s",target);
    sscanf(info[i + 2],"%s%s",predict_starttime_date,predict_starttime_time);
    predict_starttime = metis_strptime(predict_starttime_date, predict_starttime_time);//���ַ���ʱ��ת����ʱ���
    sscanf(info[i + 3],"%s%s",predict_endtime_date,predict_endtime_time);
    predict_endtime = metis_strptime(predict_endtime_date, predict_endtime_time);//���ַ���ʱ��ת����ʱ���
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

//��ȡѵ�����ݼ��������
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

//Ԥ�����ݣ�����Ԥ�����ݣ�������Ԥ��ͻ���Ҫ�ֿ�
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

//ɸѡ��Ŀ��Ҫ��Ԥ������
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

//�������������
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
	
	int utilization_rate;//������
	while(vps_type_num!0)
	{
		int sum_cpu = 0;
		int sum_mem = 0;
	    for(int i = 0; i < vps_type_num; i ++)
	    {
	        sum_cpu = sum_cpu + vps_type[i].cpu * dispose[ps_num][i];//����
	        sum_mem = sum_mem + vps_type[i].mem * dispose[ps_num][i];
	    }

	    //����cpu���ڴ������̶�
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
	    					//״̬����
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
	    						//����w1,w2,u1,u2

	    						w1=sqrt((pow(1-utilization_rate[s][cpu]),2)+(pow(1-utilization_rate[s][mem]),2));
	    						w2=sqrt((pow(1-utilization_rate[s+1][cpu]),2)+(pow(1-utilization_rate[s+1][mem]),2));
	    						u1=sqrt((pow(utilization_rate[all][cpu]-utilization_rate[s][cpu]),2)+(pow(utilization_rate[all][mem]-utilization_rate[s][mem]),2));
	    						u1=sqrt((pow(utilization_rate[all][cpu]-utilization_rate[s+1][cpu]),2)+(pow(utilization_rate[all][mem]-utilization_rate[s+1][mem]),2));
	    						//�������õ�i+1̨������ľ�������
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
	    	dispose[ps_num][i]=f_cpu[i][j][m];`		//����
	    	delete(vps_type_num[i]);
	    	remain_vps_mem=sum_mem-vps_type.mem;
	    	remain_vps_cpu=sum_cpu-vps_type.cpu;
	    }
	}
}














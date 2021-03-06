 /*
* Copyright(C) 2011,Hikvision Digital Technology Co., Ltd 
* 
* File   name��main.cpp
* Discription��demo for muti thread get stream
* Version    ��1.0
* Author     ��luoyuhua
* Create Date��2011-12-10
* Modification History��
*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "HCNetSDK.h"
#include "iniFile.h"
#include <fstream>
#include <iostream>
#include <string>
#include <sys/time.h>
#include <time.h>
using namespace std;

typedef struct tagREAL_PLAY_INFO
{
	char szIP[16];
	int iUserID;
	int iChannel;
}REAL_PLAY_INFO, *LPREAL_PLAY_INFO;

string *CameraTask = new string[50];
int Num=0;
int CameraID[50];
int CameraChannel[50];
NET_DVR_JPEGPARA CameraJpegParas[50];
char**CameraPath = new char*[50];
char* BASEDIR="../human_count_project/camera/";
void GetCamera()
{
	ifstream in;
	char filename[200];
	//char filename = "../human_count_project/CameraList.txt";
	sprintf(filename,"../human_count_project/CameraList.txt");
	string s;
	in.open(filename);
	while (getline(in,s))
	{
		CameraTask[Num] = s.c_str();
		cout << CameraTask[Num]<< endl;
		Num++;
	}
}

void OpenCamera()
{
	for(int i=0;i<Num;i++)
	{
		char *TaskID = new char[100];
		sprintf(TaskID,"%s",CameraTask[i].c_str());

		//char DeviceDir[] = "../human_count_project/camera/";
		char *DeviceDir=new char[400];
		sprintf(DeviceDir,"%s%s/config/Device.ini",BASEDIR,TaskID);
		IniFile ini(DeviceDir);  //读取配置文件
		unsigned int dwSize = 0;
		char sSection[16] = "DEVICE";

		char *sIP = ini.readstring(sSection, "ip", "error", dwSize);
		int iPort = ini.readinteger(sSection, "port", 0);
		char *sUserName = ini.readstring(sSection, "username", "error", dwSize);
		char *sPassword = ini.readstring(sSection, "password", "error", dwSize);
		int iChannel = ini.readinteger(sSection, "channel", 0);

		NET_DVR_DEVICEINFO_V30 struDeviceInfo;  //设备参数结构体
		int iUserID = NET_DVR_Login_V30(sIP, iPort, sUserName, sPassword, &struDeviceInfo); //相机注册
		for(int i=0;iUserID==-1&&i<5;i++){
			iUserID = NET_DVR_Login_V30(sIP, iPort, sUserName, sPassword, &struDeviceInfo);
			
		}	



		DWORD dwRet = 0;
		NET_DVR_COMPRESSIONCFG_V30 struCompress={0};
		bool succ = NET_DVR_GetDVRConfig(iUserID, NET_DVR_GET_COMPRESSCFG_V30, iChannel, &struCompress, sizeof(struCompress), &dwRet);
		if(succ==false){
			printf("Error %d\n",NET_DVR_GetLastError());
		}else{
			printf("GetDeviceAbility %s-Reso:%d\n\n",TaskID,struCompress.struNetPara.byResolution);
		}
		
		char szInBuf[1024]={0};
		char *pOutBuf = new char[1024*1024*10];
		memset(pOutBuf,0,1024*1024*10);
		sprintf(szInBuf,"<AudioVideoCompressInfo> \
			<AudioChannelNumber>1</AudioChannelNumber> \			
			<VoiceTalkChannelNumber>1</VoiceTalkChannelNumber> \			
			<VideoChannelNumber>%d</VideoChannelNumber>  \ 	
			</AudioVideoCompressInfo>",iChannel);
		if(NET_DVR_GetDeviceAbility(iUserID, DEVICE_ENCODE_ALL_ABILITY_V20,szInBuf,(DWORD)strlen(szInBuf), pOutBuf,1024*1024*10)){
			if(iChannel==15)	printf("ALLAbility: %s\n",pOutBuf);	
		}else{
			printf("ALLAbility Error %d\n",NET_DVR_GetLastError());
		}

		string sxml=pOutBuf;
		string reso2="704*576(4CIF)";
		string::size_type idx = sxml.find(reso2);

		NET_DVR_JPEGPARA JpegPara;
		if(idx!=string::npos){
			printf("Has 704*576(4CIF)\n");
			JpegPara  = {2,0};
		}
		else{
			printf("Doesn't have 704*576(4CIF)\n");
			JpegPara  = {0xff,0};
		}
		
		

		//char ImagePath[] = "../human_count_project/camera/";
		//strcat(ImagePath,TaskID);
		//strcat(ImagePath,"/");

		CameraID[i] = iUserID;
		CameraChannel[i] = iChannel;
		CameraJpegParas[i] = JpegPara;
		CameraPath[i]=new char[400];
		sprintf(CameraPath[i],"%s%s/",BASEDIR,TaskID);
		//CameraPath[i] = ImagePath;
		//printf("ImagePath in open camera:%s\n",ImagePath);
		cout << CameraID[i]<< endl;
		cout << CameraChannel[i] << endl;
	}
}

void GetJPG()
{
	while(1)
	{
		for(int i=0;i<Num;i++)
		{
			time_t t=time(0);
			char tmp[64];
			strftime(tmp, sizeof(tmp), "%Y%m%d-%H%M%S", localtime(&t));

            struct timeval t_start,t_end;
            long cost_time = 0;
            //get start time
            gettimeofday(&t_start, NULL);

			//char* imagePath="";
			char  imagePath[200];
			time_t timer = time(NULL);
			sprintf(imagePath,"%s%s.src.jpg",CameraPath[i], tmp);
//			imagePath=strcat(imagePath,ctime(&timer));
//			imagePath=strcat(imagePath,".src.jpg");
//			imagePath=strcat(CameraPath[i],imagePath);
			printf("Save Image into %s\n",imagePath);

			CameraJpegParas[i] = {0xff,0};
			int CaptureImg = NET_DVR_CaptureJPEGPicture(CameraID[i],CameraChannel[i],&CameraJpegParas[i],imagePath);
			if(CaptureImg==0){
				printf("Capture 0xff %d:%d failed, error = %d\n", CameraID[i], CameraChannel[i], NET_DVR_GetLastError());
				CameraJpegParas[i] = {7,0};
				CaptureImg = NET_DVR_CaptureJPEGPicture(CameraID[i],CameraChannel[i],&CameraJpegParas[i],imagePath);
				if(CaptureImg==0){
				printf("Capture XVGA(1280*960) %d:%d failed, error = %d\n", CameraID[i], CameraChannel[i], NET_DVR_GetLastError());
				CameraJpegParas[i] = {5,0};
				CaptureImg = NET_DVR_CaptureJPEGPicture(CameraID[i],CameraChannel[i],&CameraJpegParas[i],imagePath);
				if(CaptureImg==0){
				printf("Capture HD720P(1280*720) %d:%d failed, error = %d\n", CameraID[i], CameraChannel[i], NET_DVR_GetLastError());
				CameraJpegParas[i] = {2,0};
				CaptureImg = NET_DVR_CaptureJPEGPicture(CameraID[i],CameraChannel[i],&CameraJpegParas[i],imagePath);
				if(CaptureImg==0){
				printf("Capture 4CIF(704*576) %d:%d failed, error = %d\n", CameraID[i], CameraChannel[i], NET_DVR_GetLastError());
				}
				}
				}
			}
		}
		sleep(5);
	}
}

int main()
{
    NET_DVR_Init();
	NET_DVR_SetLogToFile(3, "./record/");
	
	GetCamera();
	OpenCamera();
	GetJPG();

	char c = 0;
	while('q' != c)
	{
		printf("input 'q' to quit\n");
		printf("input: ");
		scanf("%c", &c);
	}


    NET_DVR_Cleanup();
    return 0;
}



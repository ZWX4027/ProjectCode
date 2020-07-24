#pragma once
#include<iostream>
#include<stdlib.h>
#include<vector>
#include<sstream>
#include<stdint.h>
#include<boost/filesystem.hpp>
#include<fstream>//文件
#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Iphlpapi.h>//获取网卡信息接口的头文件
#pragma comment(lib, "ws2_32.lib")//Windows下的Socket头文件
#pragma comment(lib, "Iphlpapi.lib")//获取包含网卡接口信息的库文件

#else
//Linux头文件

#endif

//客户端RangeDownload()函数中，将字符串转化为数字
class StringUtil
{
public:
	static int Str2Dig(const std::string &num)
	{
		std::stringstream tmp;
		tmp << num;
		int res;
		tmp >> res;
		return res;
	}
};

//设计一个文件操作工具类, 目的是为了让外部直接使用文件操作接口即可;
//若后期文件操作有其它修改, 则只需要修改文件操作I具类即可, 而不需要对原文进行改变
class FileUtil
{
public:
	static uint64_t GetFilesize(const std::string &name)
	{
		return boost::filesystem::file_size(name);
	}

	//向指定文件的指定位置写入指定数据
	static bool Write(const std::string &name, const std::string &body, int offset = 0)
	{
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "wb+");
		if (fp == NULL)
		{
			std::cerr << "打开文件失败\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);
		size_t ret = fwrite(body.c_str(), 1, body.size(), fp);
		if (ret != body.size())
		{
			std::cerr << "向文件写入数据失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}

	//指针参数表示这是一个输出型参数
	//const & 表示这是一个输入型参数
	//&表示这是一个这是一个输入输出型参数
	static bool Read(const std::string &name, std::string *body)//读取整个文件数据
	{
		//boost库的功能：通过文件名获取文件大小
		uint64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		std::cout << "读取文件数据:" << name << "size:" << filesize << "\n";
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL)
		{
			std::cerr << "打开文件数据失败\n";
			return false;
		}
		size_t ret = fread(&(*body)[0], 1, filesize, fp);
		if (ret != filesize)
		{
			std::cerr << "读取文件失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}

	//从name文件中，从offset开始读取len长度的数据放在body中
	static bool ReadRange(const std::string &name, std::string  *body, int len, int offset)
	{
		body->resize(len);
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL)
		{
			std::cerr << "打开文件数据失败\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);//跳转到指定位置
		size_t ret = fread(&(*body)[0], 1, len, fp);
		if (ret != len)
		{
			std::cerr << "读取文件失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}

	//获取分块下载
	static bool GetRange(const std::string& range_str, int* start, int* end)
	{
		size_t  pos1 = range_str.find('-');
		size_t pos2 = range_str.find('=');
		*start = std::atol(range_str.substr(pos2 + 1, pos1 - pos2 - 1).c_str());
		std::cout << "range_str.substr(pos1 + 1, pos1 - pos2 - 1):" << range_str.substr(pos1 + 1, pos1 - pos2 - 1) << std::endl;
		*end = std::atol(range_str.substr(pos1 + 1).c_str());
		std::cout << "range_str.substr(pos1 + 1):" << range_str.substr(pos1 + 1) << std::endl;
		return true;
	}
};

class Adapter
{
public:
	uint32_t _ip_addr; //网卡，上的IP地址
	uint32_t _mask_addr;//网卡上的子网掩码
};

class AdapterUtil
{
public:
#ifdef _WIN32
	//windows下获取所有网卡信息实现
	static bool  GetAllAdapter(std::vector<Adapter> *list)
	{
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();//开辟一块网卡信息结构的空间
		//IP_ADAPTER_INFO 存放网卡信息的结构体
		//GetAdaptersInfo  windows下获取网卡信息接口信息--网卡信息可能有多个--因此传入一个指针；但是结构体它里面只能保存一个信息
		
		//获取网卡信息有可能失败
		//因为空间不足可能失败，因此有一个输出型参数，用于向用户返回所有网卡信息空间所用到的大小

		uint64_t all_adapter_size = sizeof(PIP_ADAPTER_INFO); //all_adapter_size是一个输出型参数，用于获取实际所有网卡信息的所占空间大小
		int ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);
	
		if (ret == ERROR_BUFFER_OVERFLOW)//这个错误表示缓冲区溢出（空间不足）
		{
			delete p_adapters;//把刚才开辟的空间释放
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapter_size];//重新申请一个返回的网卡信息大小的空间
			GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);//重新获取网卡信息
		}

		while (p_adapters)
		{

			Adapter adapter; 
			//inet_addr只能转化ipv4的网络地址,不能用
			//inet_pton将一个字符串点分十进制的IP地址转换为网络字节序IP地址
			inet_pton(AF_INET, p_adapters->IpAddressList.IpAddress.String, &adapter._ip_addr);
			inet_pton(AF_INET, p_adapters->IpAddressList.IpMask.String, &adapter._mask_addr);
			if (adapter._ip_addr != 0) //因为某些网卡并没有启用，导致ip地址为0
			{
				list->push_back(adapter); //将网卡信息添加到vector中返回给用户

				std::cout << "网卡名称:" << p_adapters->AdapterName << std::endl;
				std::cout << "网卡描述:" << p_adapters->Description << std::endl;
				std::cout << "IP地址:" << p_adapters->IpAddressList.IpAddress.String << std::endl;
				std::cout << "子网掩码 : " << p_adapters->IpAddressList.IpMask.String << std::endl;
				std::cout << std::endl;
			}
			p_adapters = p_adapters->Next;
		}
		delete p_adapters;
		return true;
	}

#else
	//Linux下获取网卡信息实现
	bool GetAllAdapter(std::vector<Adapter> *list);
	{
		return true;
	}
#endif
};



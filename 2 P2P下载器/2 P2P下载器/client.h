#pragma once
#include<thread>
#include<boost/filesystem.hpp>
#include"util.h"
#include"httplib.h"
#pragma comment(lib, "ws2_32.lib")

#define P2P_PORT 9000  //端口信息
#define MAX_IPBUFFER 16
#define MAX_RANGE  (100*1024*1024)
#define SHREAD_PATH "./Shread/"
#define DOWNLOAD_PATH "./download/"

class Host
{
public:
	uint32_t  _ip_addr; //要配对的主机IP地址
	bool  _pair_ret;   //用于存放配对结果，配对成功则为true，失败则为false；
};

class Client
{
public:
	bool Start()
	{
		//客户端程序需要循环运行，因为下载文件不是只下再一次
		//循环下载每次下载一个文件之后都会进行主机配对并不合理
		while (1)
		{
			GetonlineHost();
		}
		return true;
	}
	//主机配对线程入口函数
	void HostPair(Host *host)
	{
		//1.组织HTTP协议格式的请求数据
		//2.搭建一个TCP客户端，将数据发送
		//3.等待服务器端的回复，并进行解析
		
		//这个过程使用第三方库httplib实现，注：此处须知httplib的总体上实现流程
		host->_pair_ret = false;
		char buf[MAX_IPBUFFER] = { 0 };//缓冲区——用于接收转换后的字符串
		//inet_ntop(地址结构,IP地址,缓冲区,缓冲区大小)
		inet_ntop(AF_INET, &host->_ip_addr, buf, MAX_IPBUFFER);
		httplib::Client cli(buf, P2P_PORT); //实例化httplib客户端对象
		auto rsp = cli.Get("/hostpair");  //向服务端发送资源为/hostpair的Get请求，若连接建立失败，Get会返回NULL
		if (rsp && rsp->status == 200) //判断响应结果是否正确 
		{
			host->_pair_ret = true;   //重置主机配对结果
		}
		return;
	}

	//1.获取在线主机
	bool GetonlineHost()
	{
		char ch = 'Y'; //是否重新匹配，默认是进行匹配的，若已经匹配过，online主机不为空，则让用户选择
		if (!_online_host.empty())
		{
			std::cout << "是否重新配对主机(Y/N)";
			fflush(stdout);
			std::cin >> ch;
		}
		if (ch == 'Y')
		{
			std::cout << "开始主机匹配...\n";
			//（1）获取网卡信息，进而获得所有局域网中所有IP地址列表信息
			std::vector<Adapter> list;
			AdapterUtil::GetAllAdapter(&list);

			//获取所有主机IP地址，添加到host_list中
			std::vector<Host> host_list;//创建一个vector, 先将所有主机(所有网卡的主机)的地址实例化出一个对象，添加到vector中
			for (int i = 0; i < list.size(); ++i)//得到所有主机IP地址列表
			{
				uint32_t ip = list[i]._ip_addr;
				uint32_t mask = list[i]._mask_addr;

				//计算网络号
				uint32_t net = (ntohl(ip & mask));
				//计算最大主机号（子网掩码取反）
				uint32_t max_host = (~ntohl(mask));//这个主机IP的计算应该使用主机字节序(小端)的网络号和主机号
				for (int j = 1; j < (int32_t)max_host; ++j)
				{
					uint32_t host_ip = net + j;  //主机的IP地址=网络号+主机号
					Host host;
					host._ip_addr = htonl(host_ip);//将主机字节序转换为网络字节序
					host._pair_ret = false;
					host_list.push_back(host);
				}
			}
			//在for循环中创建线程std::thread,实例化出来的对象是一个局部对象，循环起来就会被释放，无法在循环之外集中等待，所以在for循环外部创建一 个vector< std::thread*>list;
			//对Host_list中主机创建线程进行配对，取指针是因为std::thread是一个局部变量，为了防止完成后被释放
			std::vector<std::thread*> thr_list(host_list.size());
			for (int i = 0; i < host_list.size(); ++i)
			{
				//当线程的入口函数，是一个类的成员函数的时候，需要注意第一个参数&Client::的写法
				thr_list[i] = new std::thread(&Client::HostPair, this, &host_list[i]);
				//实例化的线程对象空间在堆上申请，这样的话for循环运行完，则不会被自动释放

			}
			std::cout << "正在主机匹配中，请稍后....\n";
			//等待所有主机线程配对完毕，判断配对结果，将在线主机添加到_online_host中
			for (int i = 0; i < host_list.size(); ++i)
			{
				thr_list[i]->join();//等待一个线程的退出，线程退出，主机配对却是不一定成功的
				if (host_list[i]._pair_ret == true)
				{
					_online_host.push_back(host_list[i]);
				}
				delete thr_list[i];
			}
		}
		//打印在线主机列表，供用户选择
		for (int i = 0; i < _online_host.size(); ++i)
		{
			char buf[MAX_IPBUFFER] = { 0 };
			inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, MAX_IPBUFFER);
			std::cout << "\t" << buf << std::endl;
		}
		//（2）逐个对IP地址表发送配对请求
		//（3）若配对请求得到响应，则对应在线主机，则将IP添加到_online_host列表中
		std::cout << "请选择配对主机，获取共享文件列表" << std::endl;
		//查看目录是否存在，不存在则创建目录
		if (!boost::filesystem::exists(SHREAD_PATH))
		{
			boost::filesystem::create_directory(SHREAD_PATH);
		}
		fflush(stdout);
		std::string select_ip;
		std::cin >> select_ip;
		GetShareList(select_ip); //用户选择主机之后，调用获取文件列表接口
		return true;
	}

	//2.获取文件列表
	bool GetShareList(const std::string &host_ip)
	{
		//向服务端发送一个文件列表获取请求
		//1.先发送请求
		//2.得到相应之后，解析正文(文件名称)

		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		//查看目录是否存在，不存在则创建目录
		if (!boost::filesystem::exists(SHREAD_PATH))
		{
			boost::filesystem::create_directory(SHREAD_PATH);
		}
		auto rsp = cli.Get("/list");
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "获取文件列表响应错误\n" << std::endl;
			return false;
		}

		//打印正文--》打印服务端响应的文件名称列表，供用户选择
		//body:filename1\r\nfilename
		std::cout << rsp->body << std::endl;
		std::cout << "\n请选择要下载的文件";
		fflush(stdout);
		std::string filename;
		std::cin >> filename;
		RangeDownload(host_ip, filename);
		return true;
	}

	//3.下载文件
	//若文件一次性下载，遇到大文件比较危险
	bool DownloadFile(const std::string &host_ip, const std::string& filename)
	{
		//1.向服务端发送文件下载请求--》filename
		//2.得到响应结果，响应中的body正文就是文件数据
		//3.创建文件，将文件写入文件中，关闭文件
		std::string req_path = "/download/" + filename;  //得到响应数据中是没有/filename中的/的
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		std::cout << "向服务端发送文件下载请求:" << host_ip << req_path << std::endl;
		auto rsp = cli.Get(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "下载文件获取响应失败/n";
			return false;
		}
		std::cout << "获取下载文件响应成功\n";
		if (!boost::filesystem::exists(DOWNLOAD_PATH))
		{
			boost::filesystem::create_directory(DOWNLOAD_PATH);
		}
		std::string realpath = DOWNLOAD_PATH + filename;
		if (FileUtil::Write(realpath, rsp->body) == false)
		{
			std::cerr << "文件下载失败\n";
			return false;
		}
		std::cout << "下载文件成功\n";
		return true;
	}

	//分块传输（防止文件过大造成内存耗尽和效率低下的问题）
	bool RangeDownload(const std::string &host_ip, const std::string &filename)
	{
		//1.发送Head请求，通过响应中的Content_Length获取文件大小	
		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Head(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cout << "获取文件大小失败\n";
			return false;
		}
		//httplib中：response类中的成员函数
		//std.string get_header_value(std:string &key)通过http头部信息字段名获取值
		std::string clen = rsp->get_header_value("Content-Length");
		int filesize = StringUtil::Str2Dig(clen);

		//2.根据文件大小进行分块
		//int range_count = filesize / MAX_RANGE;
		//a.若文件大小小于块大小，则直接下载文件
		if (filesize < MAX_RANGE)
		{
			std::cout << "文件较小，直接下载文件\n";
			return DownloadFile(host_ip, filename);
		}
		//b.若文件大小不能整除块大小,则分块个数为文件大小除以块大小+1
		std::cout << "文件过大，分块进行下载\n";
		std::cout << filesize << std::endl;
		int range_count = 0;
		if (filesize % MAX_RANGE == 0)
		{
			range_count = filesize / MAX_RANGE;
		}
		else
		{
			range_count = (filesize / MAX_RANGE) + 1;
		}
		int range_start = 0, range_end = 0;
		for (int i = 0; i < range_count; i++)
		{
			range_start = i * MAX_RANGE;
			if (i == (range_count - 1))
			{//末尾分块
				range_end = filesize - 1;
			}
			else
			{
				range_end = ((i + 1) * MAX_RANGE) - 1;
			}
			std::cout << "客户端请求分块下载" << range_start << "-" << range_end << std::endl;
			//c.若文件大小刚好整除块大小，则分块个数为文件大小除以块大小
			//3.逐一请求分块区间的数据，得到响应后写入文件的指定位置

			//header.insert(httplib::make_range_header({ { range_start, range_end } }));//设置一个range区间
			std::stringstream tmp;
			tmp << "byte=" << range_start << "-" << range_end;
			httplib::Client cli(host_ip.c_str(), P2P_PORT);
			httplib::Headers header;
			header.insert(std::make_pair("Range", tmp.str()));
			auto rsp = cli.Get(req_path.c_str(), header);//向服务端发送一个分段请求
			if (rsp == NULL || rsp->status != 206)
			{
				std::cout << "区间下载文件失败\n";
				return false;
			}
			std::string real_path = DOWNLOAD_PATH + filename;
			if (!boost::filesystem::exists(DOWNLOAD_PATH))
			{
				boost::filesystem::create_directory(DOWNLOAD_PATH);
			}
			//获取文件成功，向文件中分块写入数据
			FileUtil::Write(real_path, rsp->body, range_start);
		}
		std::cout << "文件下载成功\n";
		return true;
	}

private:
	std::vector<Host> _online_host;
};


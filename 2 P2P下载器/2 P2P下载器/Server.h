#pragma once
#include<thread>
#include<boost/filesystem.hpp>
#include"util.h"
#include"httplib.h"

#define P2P_PORT 9000
#define MAX_IPBUFFER 16
#define MAX_RANGE  (100*1024*1024)
#define SHREAD_PATH "./Shread/"
#define DOWNLOAD_PATH "./download/"


class Server
{
public:
	bool Start()
	{
		//添加对客户端请求的处理方式对应关系
		_srv.Get("/hostpair", HostPair);
		_srv.Get("/list", ShareList);

		//正则表达式：将特殊字符以指定的格式，表示具有关键特征的数据
		//正则表达式中：.匹配除\r或\n之外的任意字符，*表示匹配前边的字符任意次
		//防止与上方的请求发生冲突，因此在请求中加入download路径
		_srv.Get("/download/.*", Download);
		_srv.listen("0.0.0.0", P2P_PORT);
		return true;
	}

private:
	static void HostPair(const httplib::Request &req, httplib::Response &rsp)
	{
		rsp.status = 200;
		return;
	}

	//获取共享文件列表，在主机上设置一个共享目录，凡是这个目录下的文件都是要给别人共享的
	static void ShareList(const httplib::Request &req, httplib::Response &rsp)
	{
		//1.查看目录是否存在，不存在则创建目录
		//2.浏览这个目录下的文件信息
			//opendir/readdir/closedir或scandir，则选择使用c + +boost库中提供的文件系统接口实现目录浏览，以及文件的各种操作I
			//因为VS3013系统中默认并不包含boost库，因此需要用户自己安装,然后在项目中引入头文件和库即可
			//
		if (!boost::filesystem::exists(SHREAD_PATH))
		{
			boost::filesystem::create_directory(SHREAD_PATH);
		}
		boost::filesystem::directory_iterator begin(SHREAD_PATH);// 实例化目录迭代器
		boost::filesystem::directory_iterator end;   //实例化迭代器的末尾
		//开始迭代目录
		for (; begin != end; ++begin)
		{
			if (boost::filesystem::is_directory(begin->status()))
			{
				//当前版本我们只获取文件名称，不做多层级目录操作
				continue;
			}
			std::string name = begin->path().filename().string();  //只要文件名，不要路径
			rsp.body += name + "\r\n";  //filename1\r\n filename2\r\n
		}
		rsp.status = 200;
		return;
	}

	//boost::filesystem::path().filename()只获取文件名称，abc/filename.txt
	static void Download(const httplib::Request &req, httplib::Response &rsp)
	{
		std::cout << "服务端收到文件下载请求:" << req.path << std::endl;
		//req.path---客户端请求的资源路径   /download/filename.txt
		boost::filesystem::path req_path(req.path);
		std::string name = req_path.filename().string(); //只获取文件名称；  filename.txt
		std::cout << "服务端收到实际的文件下载名称:" << name << std::endl;
		std::string realpath = SHREAD_PATH + name;  //实际文件路径应该是在共享目录下的
		std::cout << "服务端收到实际的文件下载路径:" << realpath << std::endl;
		//boost::filesystem::exists()判断文件是否存在
		if (!boost::filesystem::exists(realpath) || boost::filesystem::is_directory(realpath))
		{
			rsp.status = 404;//不存在
			return;
		}
		//在httplib中，Get接口不但针对客户端的GET请求方法，也针对客户端的HEAD请求方法
		//httplib中的GET/HEAD处理
		if (req.method == "GET")
		{//以前GET请求就是直接下载完整文件，但是现在不一样了，现在有了分块传输这个功能
			//判断是否是分块传输的依据，就是这次请求中是否有range头部字段
			if (req.has_header("Range"))
			{//判断请求头中是否包含Range字段
				//这就是一个分块传输
				//需要知道分块区间是多少
				std::string range_str = req.get_header_value("Range");
				int range_start;
				int range_end;
				FileUtil::GetRange(range_str, &range_start, &range_end);
				int range_len = range_end - range_start + 1;//区间长度

				std::cout << "Range:" << range_start << "-" << range_end << std::endl;
				FileUtil::ReadRange(realpath, &rsp.body, range_len, range_start);
				rsp.status = 206;
				std::cout << "服务端响应区间数据完毕\n";
			}
			else
			{
				//没有Range头部，则是一个完成的文件下载
				if (FileUtil::Read(realpath, &rsp.body) == false)
				{
					rsp.status = 500;
					return;
				}
				rsp.status = 200;
			}
		}
		else
		{
			//这个是针对HEAD请求的客户端只要头部不要正文
			uint64_t filesize = FileUtil::GetFilesize(realpath);
			rsp.set_header("Content-Length", std::to_string(filesize));//设置响应header头部信息接口
			rsp.status = 200;
		}
		return;
	}
private:
	httplib::Server _srv;
};
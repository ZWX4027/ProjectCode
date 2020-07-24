#include"util.h"
#include"httplib.h"
#include"Client.h"
#include"Server.h"

void CilentRun()
{
	Client cli;
	cli.Start();
}

//4、项目测试
int main(int argc, char *argv[])
{
	//在主线程中运行客户端模块以及服务端模块
	//创建一个线程运行客户端模块，主线程运行服务端模块
	std::thread thr_client(CilentRun);  //若客户端运行的时候，服务端还没有启动	
	Server srv;
	srv.Start();
	system("pause");
	return 0;
}

//3、测试boost库,使用c++目录迭代器，浏览目录
//void Scandir()
//{
//	//boost::filesystem::path().filename() //只获取文件名称；abc/filename.txt -->filename.txt
//	//boost::filesystem::exists() //判断文件是否存在
//
//	const char *ptr = "./";
//	boost::filesystem::directory_iterator begin(ptr);  //定义一个目录迭代器
//	boost::filesystem::directory_iterator end;
//	for (; begin != end; ++begin)
//	{
//		//begin->status()  目录中当前文件的状态信息
//		//boost::filesystem::is_directory() 判断当前文件是否是一个目录
//		if (boost::filesystem::is_directory(begin->status()))
//		{
//			//begin->path().string()  获取当前文件的文件迭代名
//			std::cout << begin->path().string() << "s是一个目录\n";
//		}
//		else
//		{
//			std::cout << begin->path().string() << "是一个普通文件\n";
//			//begin->path().filename()  只获取文件路径名中的文件名称，而不要路径
//			std::cout << "文件名:" << begin->path().filename().string() << std::endl;
//		}
//	}
//}
//int main()
//{
//	Scandir();
//	return 0;
//}

//2、测试httplib
//void helloworld(const httplib::Request &req, httplib::Response &rsp)
//{
//	//printf(" httplib server recv a req:%s\n", req.path.c_str());
//	rsp.set_content("<html><h1>HelloWorld</h1></html>","text/html");
//	rsp.status = 200;
//}
//int main(int argc,char *argv[])
//{
//	httplib::Server srv;
//	srv.Get("/",helloworld);
//	srv.listen("0.0.0.0",9000);
//	return 0;
//}

//1、测试util获取网卡信息
//int main(int argc,char *argv[])
//{
//	std::vector<Adapter>list;
//	AdapterUtil::GetAllAdapter(&list);
//	return 0;
//}


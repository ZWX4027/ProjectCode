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
		//��ӶԿͻ�������Ĵ���ʽ��Ӧ��ϵ
		_srv.Get("/hostpair", HostPair);
		_srv.Get("/list", ShareList);

		//������ʽ���������ַ���ָ���ĸ�ʽ����ʾ���йؼ�����������
		//������ʽ�У�.ƥ���\r��\n֮��������ַ���*��ʾƥ��ǰ�ߵ��ַ������
		//��ֹ���Ϸ�����������ͻ������������м���download·��
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

	//��ȡ�����ļ��б�������������һ������Ŀ¼���������Ŀ¼�µ��ļ�����Ҫ�����˹����
	static void ShareList(const httplib::Request &req, httplib::Response &rsp)
	{
		//1.�鿴Ŀ¼�Ƿ���ڣ��������򴴽�Ŀ¼
		//2.������Ŀ¼�µ��ļ���Ϣ
			//opendir/readdir/closedir��scandir����ѡ��ʹ��c + +boost�����ṩ���ļ�ϵͳ�ӿ�ʵ��Ŀ¼������Լ��ļ��ĸ��ֲ���I
			//��ΪVS3013ϵͳ��Ĭ�ϲ�������boost�⣬�����Ҫ�û��Լ���װ,Ȼ������Ŀ������ͷ�ļ��Ϳ⼴��
			//
		if (!boost::filesystem::exists(SHREAD_PATH))
		{
			boost::filesystem::create_directory(SHREAD_PATH);
		}
		boost::filesystem::directory_iterator begin(SHREAD_PATH);// ʵ����Ŀ¼������
		boost::filesystem::directory_iterator end;   //ʵ������������ĩβ
		//��ʼ����Ŀ¼
		for (; begin != end; ++begin)
		{
			if (boost::filesystem::is_directory(begin->status()))
			{
				//��ǰ�汾����ֻ��ȡ�ļ����ƣ�������㼶Ŀ¼����
				continue;
			}
			std::string name = begin->path().filename().string();  //ֻҪ�ļ�������Ҫ·��
			rsp.body += name + "\r\n";  //filename1\r\n filename2\r\n
		}
		rsp.status = 200;
		return;
	}

	//boost::filesystem::path().filename()ֻ��ȡ�ļ����ƣ�abc/filename.txt
	static void Download(const httplib::Request &req, httplib::Response &rsp)
	{
		std::cout << "������յ��ļ���������:" << req.path << std::endl;
		//req.path---�ͻ����������Դ·��   /download/filename.txt
		boost::filesystem::path req_path(req.path);
		std::string name = req_path.filename().string(); //ֻ��ȡ�ļ����ƣ�  filename.txt
		std::cout << "������յ�ʵ�ʵ��ļ���������:" << name << std::endl;
		std::string realpath = SHREAD_PATH + name;  //ʵ���ļ�·��Ӧ�����ڹ���Ŀ¼�µ�
		std::cout << "������յ�ʵ�ʵ��ļ�����·��:" << realpath << std::endl;
		//boost::filesystem::exists()�ж��ļ��Ƿ����
		if (!boost::filesystem::exists(realpath) || boost::filesystem::is_directory(realpath))
		{
			rsp.status = 404;//������
			return;
		}
		//��httplib�У�Get�ӿڲ�����Կͻ��˵�GET���󷽷���Ҳ��Կͻ��˵�HEAD���󷽷�
		//httplib�е�GET/HEAD����
		if (req.method == "GET")
		{//��ǰGET�������ֱ�����������ļ����������ڲ�һ���ˣ��������˷ֿ鴫���������
			//�ж��Ƿ��Ƿֿ鴫������ݣ���������������Ƿ���rangeͷ���ֶ�
			if (req.has_header("Range"))
			{//�ж�����ͷ���Ƿ����Range�ֶ�
				//�����һ���ֿ鴫��
				//��Ҫ֪���ֿ������Ƕ���
				std::string range_str = req.get_header_value("Range");
				int range_start;
				int range_end;
				FileUtil::GetRange(range_str, &range_start, &range_end);
				int range_len = range_end - range_start + 1;//���䳤��

				std::cout << "Range:" << range_start << "-" << range_end << std::endl;
				FileUtil::ReadRange(realpath, &rsp.body, range_len, range_start);
				rsp.status = 206;
				std::cout << "�������Ӧ�����������\n";
			}
			else
			{
				//û��Rangeͷ��������һ����ɵ��ļ�����
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
			//��������HEAD����Ŀͻ���ֻҪͷ����Ҫ����
			uint64_t filesize = FileUtil::GetFilesize(realpath);
			rsp.set_header("Content-Length", std::to_string(filesize));//������Ӧheaderͷ����Ϣ�ӿ�
			rsp.status = 200;
		}
		return;
	}
private:
	httplib::Server _srv;
};
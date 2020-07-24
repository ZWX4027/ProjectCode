#pragma once
#include<thread>
#include<boost/filesystem.hpp>
#include"util.h"
#include"httplib.h"
#pragma comment(lib, "ws2_32.lib")

#define P2P_PORT 9000  //�˿���Ϣ
#define MAX_IPBUFFER 16
#define MAX_RANGE  (100*1024*1024)
#define SHREAD_PATH "./Shread/"
#define DOWNLOAD_PATH "./download/"

class Host
{
public:
	uint32_t  _ip_addr; //Ҫ��Ե�����IP��ַ
	bool  _pair_ret;   //���ڴ����Խ������Գɹ���Ϊtrue��ʧ����Ϊfalse��
};

class Client
{
public:
	bool Start()
	{
		//�ͻ��˳�����Ҫѭ�����У���Ϊ�����ļ�����ֻ����һ��
		//ѭ������ÿ������һ���ļ�֮�󶼻����������Բ�������
		while (1)
		{
			GetonlineHost();
		}
		return true;
	}
	//��������߳���ں���
	void HostPair(Host *host)
	{
		//1.��֯HTTPЭ���ʽ����������
		//2.�һ��TCP�ͻ��ˣ������ݷ���
		//3.�ȴ��������˵Ļظ��������н���
		
		//�������ʹ�õ�������httplibʵ�֣�ע���˴���֪httplib��������ʵ������
		host->_pair_ret = false;
		char buf[MAX_IPBUFFER] = { 0 };//�������������ڽ���ת������ַ���
		//inet_ntop(��ַ�ṹ,IP��ַ,������,��������С)
		inet_ntop(AF_INET, &host->_ip_addr, buf, MAX_IPBUFFER);
		httplib::Client cli(buf, P2P_PORT); //ʵ����httplib�ͻ��˶���
		auto rsp = cli.Get("/hostpair");  //�����˷�����ԴΪ/hostpair��Get���������ӽ���ʧ�ܣ�Get�᷵��NULL
		if (rsp && rsp->status == 200) //�ж���Ӧ����Ƿ���ȷ 
		{
			host->_pair_ret = true;   //����������Խ��
		}
		return;
	}

	//1.��ȡ��������
	bool GetonlineHost()
	{
		char ch = 'Y'; //�Ƿ�����ƥ�䣬Ĭ���ǽ���ƥ��ģ����Ѿ�ƥ�����online������Ϊ�գ������û�ѡ��
		if (!_online_host.empty())
		{
			std::cout << "�Ƿ������������(Y/N)";
			fflush(stdout);
			std::cin >> ch;
		}
		if (ch == 'Y')
		{
			std::cout << "��ʼ����ƥ��...\n";
			//��1����ȡ������Ϣ������������о�����������IP��ַ�б���Ϣ
			std::vector<Adapter> list;
			AdapterUtil::GetAllAdapter(&list);

			//��ȡ��������IP��ַ����ӵ�host_list��
			std::vector<Host> host_list;//����һ��vector, �Ƚ���������(��������������)�ĵ�ַʵ������һ��������ӵ�vector��
			for (int i = 0; i < list.size(); ++i)//�õ���������IP��ַ�б�
			{
				uint32_t ip = list[i]._ip_addr;
				uint32_t mask = list[i]._mask_addr;

				//���������
				uint32_t net = (ntohl(ip & mask));
				//������������ţ���������ȡ����
				uint32_t max_host = (~ntohl(mask));//�������IP�ļ���Ӧ��ʹ�������ֽ���(С��)������ź�������
				for (int j = 1; j < (int32_t)max_host; ++j)
				{
					uint32_t host_ip = net + j;  //������IP��ַ=�����+������
					Host host;
					host._ip_addr = htonl(host_ip);//�������ֽ���ת��Ϊ�����ֽ���
					host._pair_ret = false;
					host_list.push_back(host);
				}
			}
			//��forѭ���д����߳�std::thread,ʵ���������Ķ�����һ���ֲ�����ѭ�������ͻᱻ�ͷţ��޷���ѭ��֮�⼯�еȴ���������forѭ���ⲿ����һ ��vector< std::thread*>list;
			//��Host_list�����������߳̽�����ԣ�ȡָ������Ϊstd::thread��һ���ֲ�������Ϊ�˷�ֹ��ɺ��ͷ�
			std::vector<std::thread*> thr_list(host_list.size());
			for (int i = 0; i < host_list.size(); ++i)
			{
				//���̵߳���ں�������һ����ĳ�Ա������ʱ����Ҫע���һ������&Client::��д��
				thr_list[i] = new std::thread(&Client::HostPair, this, &host_list[i]);
				//ʵ�������̶߳���ռ��ڶ������룬�����Ļ�forѭ�������꣬�򲻻ᱻ�Զ��ͷ�

			}
			std::cout << "��������ƥ���У����Ժ�....\n";
			//�ȴ����������߳������ϣ��ж���Խ����������������ӵ�_online_host��
			for (int i = 0; i < host_list.size(); ++i)
			{
				thr_list[i]->join();//�ȴ�һ���̵߳��˳����߳��˳����������ȴ�ǲ�һ���ɹ���
				if (host_list[i]._pair_ret == true)
				{
					_online_host.push_back(host_list[i]);
				}
				delete thr_list[i];
			}
		}
		//��ӡ���������б����û�ѡ��
		for (int i = 0; i < _online_host.size(); ++i)
		{
			char buf[MAX_IPBUFFER] = { 0 };
			inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, MAX_IPBUFFER);
			std::cout << "\t" << buf << std::endl;
		}
		//��2�������IP��ַ�����������
		//��3�����������õ���Ӧ�����Ӧ������������IP��ӵ�_online_host�б���
		std::cout << "��ѡ�������������ȡ�����ļ��б�" << std::endl;
		//�鿴Ŀ¼�Ƿ���ڣ��������򴴽�Ŀ¼
		if (!boost::filesystem::exists(SHREAD_PATH))
		{
			boost::filesystem::create_directory(SHREAD_PATH);
		}
		fflush(stdout);
		std::string select_ip;
		std::cin >> select_ip;
		GetShareList(select_ip); //�û�ѡ������֮�󣬵��û�ȡ�ļ��б�ӿ�
		return true;
	}

	//2.��ȡ�ļ��б�
	bool GetShareList(const std::string &host_ip)
	{
		//�����˷���һ���ļ��б��ȡ����
		//1.�ȷ�������
		//2.�õ���Ӧ֮�󣬽�������(�ļ�����)

		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		//�鿴Ŀ¼�Ƿ���ڣ��������򴴽�Ŀ¼
		if (!boost::filesystem::exists(SHREAD_PATH))
		{
			boost::filesystem::create_directory(SHREAD_PATH);
		}
		auto rsp = cli.Get("/list");
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "��ȡ�ļ��б���Ӧ����\n" << std::endl;
			return false;
		}

		//��ӡ����--����ӡ�������Ӧ���ļ������б����û�ѡ��
		//body:filename1\r\nfilename
		std::cout << rsp->body << std::endl;
		std::cout << "\n��ѡ��Ҫ���ص��ļ�";
		fflush(stdout);
		std::string filename;
		std::cin >> filename;
		RangeDownload(host_ip, filename);
		return true;
	}

	//3.�����ļ�
	//���ļ�һ�������أ��������ļ��Ƚ�Σ��
	bool DownloadFile(const std::string &host_ip, const std::string& filename)
	{
		//1.�����˷����ļ���������--��filename
		//2.�õ���Ӧ�������Ӧ�е�body���ľ����ļ�����
		//3.�����ļ������ļ�д���ļ��У��ر��ļ�
		std::string req_path = "/download/" + filename;  //�õ���Ӧ��������û��/filename�е�/��
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		std::cout << "�����˷����ļ���������:" << host_ip << req_path << std::endl;
		auto rsp = cli.Get(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "�����ļ���ȡ��Ӧʧ��/n";
			return false;
		}
		std::cout << "��ȡ�����ļ���Ӧ�ɹ�\n";
		if (!boost::filesystem::exists(DOWNLOAD_PATH))
		{
			boost::filesystem::create_directory(DOWNLOAD_PATH);
		}
		std::string realpath = DOWNLOAD_PATH + filename;
		if (FileUtil::Write(realpath, rsp->body) == false)
		{
			std::cerr << "�ļ�����ʧ��\n";
			return false;
		}
		std::cout << "�����ļ��ɹ�\n";
		return true;
	}

	//�ֿ鴫�䣨��ֹ�ļ���������ڴ�ľ���Ч�ʵ��µ����⣩
	bool RangeDownload(const std::string &host_ip, const std::string &filename)
	{
		//1.����Head����ͨ����Ӧ�е�Content_Length��ȡ�ļ���С	
		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Head(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cout << "��ȡ�ļ���Сʧ��\n";
			return false;
		}
		//httplib�У�response���еĳ�Ա����
		//std.string get_header_value(std:string &key)ͨ��httpͷ����Ϣ�ֶ�����ȡֵ
		std::string clen = rsp->get_header_value("Content-Length");
		int filesize = StringUtil::Str2Dig(clen);

		//2.�����ļ���С���зֿ�
		//int range_count = filesize / MAX_RANGE;
		//a.���ļ���СС�ڿ��С����ֱ�������ļ�
		if (filesize < MAX_RANGE)
		{
			std::cout << "�ļ���С��ֱ�������ļ�\n";
			return DownloadFile(host_ip, filename);
		}
		//b.���ļ���С�����������С,��ֿ����Ϊ�ļ���С���Կ��С+1
		std::cout << "�ļ����󣬷ֿ��������\n";
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
			{//ĩβ�ֿ�
				range_end = filesize - 1;
			}
			else
			{
				range_end = ((i + 1) * MAX_RANGE) - 1;
			}
			std::cout << "�ͻ�������ֿ�����" << range_start << "-" << range_end << std::endl;
			//c.���ļ���С�պ��������С����ֿ����Ϊ�ļ���С���Կ��С
			//3.��һ����ֿ���������ݣ��õ���Ӧ��д���ļ���ָ��λ��

			//header.insert(httplib::make_range_header({ { range_start, range_end } }));//����һ��range����
			std::stringstream tmp;
			tmp << "byte=" << range_start << "-" << range_end;
			httplib::Client cli(host_ip.c_str(), P2P_PORT);
			httplib::Headers header;
			header.insert(std::make_pair("Range", tmp.str()));
			auto rsp = cli.Get(req_path.c_str(), header);//�����˷���һ���ֶ�����
			if (rsp == NULL || rsp->status != 206)
			{
				std::cout << "���������ļ�ʧ��\n";
				return false;
			}
			std::string real_path = DOWNLOAD_PATH + filename;
			if (!boost::filesystem::exists(DOWNLOAD_PATH))
			{
				boost::filesystem::create_directory(DOWNLOAD_PATH);
			}
			//��ȡ�ļ��ɹ������ļ��зֿ�д������
			FileUtil::Write(real_path, rsp->body, range_start);
		}
		std::cout << "�ļ����سɹ�\n";
		return true;
	}

private:
	std::vector<Host> _online_host;
};


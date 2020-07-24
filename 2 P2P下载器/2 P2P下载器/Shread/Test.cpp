#include"UtilTool.h"
#include"httplib.h"
#include"Client.h"
#include"Server.h"

void Scandir()
{
	//boost::filesystem::path().filename() //ֻ��ȡ�ļ����ƣ�abc/filename.txt -->filename.txt
	//boost::filesystem::exists() //�ж��ļ��Ƿ����

	const char *ptr = "./";
	boost::filesystem::directory_iterator begin(ptr);  //����һ��Ŀ¼������
	boost::filesystem::directory_iterator end;
	for (; begin != end; ++begin)
	{
		//begin->status()  Ŀ¼�е�ǰ�ļ���״̬��Ϣ
		//boost::filesystem::is_directory() �жϵ�ǰ�ļ��Ƿ���һ��Ŀ¼
		if (boost::filesystem::is_directory(begin->status()))
		{
			//begin->path().string()  ��ȡ��ǰ�ļ����ļ�������
			std::cout << begin->path().string() << "s��һ��Ŀ¼\n";
		}
		else
		{
			std::cout << begin->path().string() << "��һ����ͨ�ļ�\n";
			//begin->path().filename()  ��ȡ�ļ�·�����е��ļ����ƣ�������·��
			std::cout << "�ļ���:" << begin->path().filename().string() << std::endl;
		}
	}
}

//void Test()
//{
//	//std::vector<Adapter> list;
//	//AdapterUtil::GetAllAdapter(&list);
//	Scandir();
//}

void CilentRun()
{
	Client cli;
	cli.Start();
}

int main(int argc, char *argv[])
{
	//�����߳������пͻ���ģ���Լ������ģ��
	//����һ���߳����пͻ���ģ�飬���߳����з����ģ��
	std::thread thr_client(CilentRun);  //���ͻ������е�ʱ�򣬷���˻�û������	
	Server srv;
	srv.Start();
	system("pause");
	return 0;
}
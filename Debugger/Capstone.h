#pragma once

#include "Capstone/include/capstone.h"
#pragma comment(lib,"capstone/capstone.lib")
#pragma comment(linker, "/NODEFAULTLIB:\"libcmtd.lib\"")

#include <windows.h>
// ����������ࣨ�����ࣩ����Ҫ������ͨ������ĵ�ַ���ػ�����

class Capstone
{
private:
	// ���ڳ�ʼ�����ڴ����ľ��
	//��Ϊ�������࣬����ʹ�þ�̬��Ա����
	static csh Handle;//��������ľ��
	static cs_opt_mem OptMem;//���÷�������ѿռ�Ľṹ��

public:
	// ����ΪĬ�Ϲ��캯��
	Capstone() = default;
	~Capstone() = default;

	// ���ڳ�ʼ���ĺ���
	static void Init();

	// ����ִ�з����ĺ��� ����������  ��Ҫ�����ĵ�ַ  �����ָ�������
	static void DisAsm(HANDLE Handle, LPVOID Addr, DWORD Count);

	//���ڻ�ȡ�쳣����һ��ָ��ĵ�ַ
	static DWORD GetExceptionNextAddress(HANDLE Handle, LPVOID Addr, DWORD Count);
};

#pragma once
#include <vector>
#include <windows.h>

using namespace std;

//ö�����ͣ����ڱ�ʾ�ϵ���Ӳ���ϵ㻹������ϵ�
enum BreakFlag {
	CcFlag, HdFlag, MemFlag
};

// �ϵ���(������): �����������Ͷϵ�����á��޸�����ԭ
// - TF �ϵ㣺�����ϵ�(����)
// - ����ϵ㣺ʹ�� int 3 ���õĶϵ�
// - Ӳ���ϵ㣺ͨ��CPU�ṩ�ĵ��ԼĴ������õĶ�\д\ִ�жϵ�(����)
// - �ڴ�ϵ㣺������ʵ�ĳһ�����ݻ��߶�ĳЩ���ݽ���д���ִ�е�ʱ�����

// ���ڱ�������ϵ���Ϣ�Ľṹ��
struct ExceptionInfo
{
	BreakFlag ExceptionFlag;//�ϵ�����
	LPVOID ExceptionAddress;//�ϵ��ַ
	BOOL EternalOrNot;//�Ƿ������öϵ�
	
	DWORD HdBpType;//Ӳ���ϵ�����
	DWORD HdBpLen;//Ӳ���ϵ��������
	
	DWORD HeaderOFMemPage;//���ڱ����ڴ�ϵ��У������ڴ�ϵ���ڴ�ҳ���׵�ַ Ҫע������ڴ������ڴ�ҳ�߽�������ֽ�֮�ڣ���ͬʱ�ı������ֽڵ��ڴ汣������
	int TypeOfMemBp;//�����ڴ�ϵ�����
	
	union
	{
		CHAR OldOpcode;//����ϵ�ԭλ�õ�OPCode
		int DRnumber;//�����ж�����Ӳ���ϵ���DR0~3���ĸ��ϵ����õģ������öϵ�ָ���ʱ��Ҫ��
		DWORD ProtectOfOld;//���ڱ����ڴ�ϵ��ԭ�ڴ汣������

	}u;
};


class BreakPoint
{
private:

	//�ñ������ڱ�ʶһ��EXCEPTION_SINGLE_STEP������Ϣ�����Ƿ����һ��һ�����ڴ�ϵ�
	static int TFAndHbp;

public:
	// ���������еĶϵ�Ķ�̬����
	static vector<ExceptionInfo> BreakPointList;

	// ����ʵ�ֵ����ϵ�: ͨ�� TF ��־λ
	static bool SetTfBreakPoint(HANDLE ThreadHandle);

	// ����ʵ������ϵ�: ͨ�� int 3(0xCC) ָ�� 
	static bool SetCcBreakPoint(HANDLE ProcessHandle, LPVOID Address);

	// �޸�һ������ϵ�  ע��int3���������쳣���ָ�ʱ��Ҫeip-1
	static bool FixCcBreakPoint(HANDLE ProcessHandle, HANDLE ThreadHandle, LPVOID Address);

	// ����ʵ��Ӳ���ϵ�: ͨ�� ���ԼĴ��� Dr0~Dr3 Dr7   ����Ĭ��Ӳ���ϵ���ִ�жϵ㣬
	// �����Ҫʵ�ֶ�д�ϵ�֮�����������Ĭ�ϲ���Ϊ0
	static bool SetHdBreakPoint(HANDLE ThreadHandle, LPVOID Address, DWORD Type = 0, DWORD Len = 0);

	// �޸�һ��Ӳ���ϵ�
	static bool FixHdBreakPoint(HANDLE ProcessHandle,HANDLE ThreadHandle, LPVOID Address, int*IsOrNot);

	//����������������ϵ�
	static bool ReSetCcBreakPointOfEnternal(HANDLE ProcessHandle, HANDLE ThreadHandle);

	//�����ڴ�ϵ�
	static bool SetMemBreakPoint(HANDLE ProcessHandle, LPVOID Address);

	//�޸��ڴ�ϵ�
	static bool FixMemBreakPoint(HANDLE ProcessHandle, HANDLE ThreadHandle ,LPVOID Address, int*IsOrNot);
	
	//���õ�������
	static bool SetPassBreakPoint(HANDLE ProcessHandle, HANDLE ThreadHandle, DWORD NextCodeAddress);
};


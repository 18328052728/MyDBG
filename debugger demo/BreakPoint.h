#pragma once
#include <vector>
#include <windows.h>
using namespace std;

enum BreakFlag {
	CcFlag, HdFlag
};

// �ϵ���(������): �����������Ͷϵ�����á��޸�����ԭ
// - TF �ϵ㣺�����ϵ�(����)
// - ����ϵ㣺ʹ�� int 3 ���õĶϵ�
// - Ӳ���ϵ㣺ͨ��CPU�ṩ�ĵ��ԼĴ������õĶ�\д\ִ�жϵ�(����)
// - �ڴ�ϵ㣺������ʵ�ĳһ�����ݻ��߶�ĳЩ���ݽ���д���ִ�е�ʱ�����

// ���ڱ�������ϵ���Ϣ�Ľṹ��
struct ExceptionInfo
{
	BreakFlag ExceptionFlag;
	LPVOID ExceptionAddress;

	union
	{
		CHAR OldOpcode;
	}u;
};

class BreakPoint
{
private:
	// ���������еĶϵ�
	static vector<ExceptionInfo> BreakPointList;

public:
	// ����ʵ�ֵ����ϵ�: ͨ�� TF ��־λ
	static bool SetTfBreakPoint(HANDLE ThreadHandle);

	// ����ʵ������ϵ�: ͨ�� int 3(0xCC) ָ��
	static bool SetCcBreakPoint(HANDLE ProcessHandle, LPVOID Address);

	// �޸�һ������ϵ�
	static bool FixCcBreakPoint(HANDLE ProcessHandle, HANDLE ThreadHandle, LPVOID Address);

	// ����ʵ��Ӳ���ϵ�: ͨ�� ���ԼĴ��� Dr0~Dr3 Dr7
	static bool SetHdBreakPoint(HANDLE ThreadHandle, LPVOID Address, DWORD Type = 0, DWORD Len = 0);

	// �޸�һ������ϵ�
	static bool FixHdBreakPoint(HANDLE ThreadHandle, LPVOID Address);
};


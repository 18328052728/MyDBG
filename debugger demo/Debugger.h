#pragma once
#include <windows.h>


// �������ࣺ ���ڴ��ļ����������ԻỰ���ȴ�������Ϣ���������
//          ��Ϣ���쳣�����ظ����Ե�����ϵͳ������

class Debugger
{
private:
	// ���ڱ�������쳣���̺߳ͽ��̾��
	HANDLE ThreadHandle = NULL;
	HANDLE ProcessHandle = NULL;

	// ���ڱ�����յ��ĵ�����Ϣ
	DEBUG_EVENT DebugEvent = { 0 };

	// �ж��Ƿ���ϵͳ�ϵ�
	BOOL isSystemBreakPoint = TRUE;

	// �����ȡ���� OEP 
	LPVOID StartAddress = 0;

	// �Ƿ���Ҫ��ȡ����
	BOOL NeddCommand = TRUE;

public:
	// �����Ե��Եķ�ʽ����һ������
	bool open(char const* FileName);

	// �ȴ������¼�
	void run();

private:
	// ר�����ڴ����쳣��Ϣ
	DWORD OnExceptionHanlder();

	// �����о��
	VOID OpenHandles();

	// �ر����о��
	VOID CloseHandles();

	// ��ȡ�û�������
	VOID GetCommand();
};


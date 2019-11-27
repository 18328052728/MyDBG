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

	// �Ƿ���Ҫ��ȡ���룬Ĭ������Ҫ�����
	BOOL NeddCommand = TRUE;

public:
	// �����Ե��Եķ�ʽ����һ������
	bool open(char const* FileName);

	bool attachment(DWORD dwPid);


	// �ȴ������¼�
	void run();

	//�ú������½�������ʼ�ռ�ر����Խ��̵������ϵ�Ĵ���
	VOID ContionBpFind();

private:
	
	//���ڱ��������ϵ��������ĸ��Ĵ���  1234�ֱ�ΪEAX EBX ECX EDX
	int ContionBpRegNumber = 0;

	//���ڱ��������ϵ�Ĵ�����ֵ
	DWORD ContionBpNumber = 0;

	//���ڱ��������ϵ��ַ����
	DWORD AddressOfContionBp = 0;


	// ר�����ڴ����쳣��Ϣ
	DWORD OnExceptionHanlder();


	// �����о��
	VOID OpenHandles();

	// �ر����о��
	VOID CloseHandles();

	// ��ȡ�û�������
	VOID GetCommand();

	//��ʾ�Ĵ�����Ϣ
	VOID ShowRegister(HANDLE ThreadHandle);
	
	//�޸Ļ�����
	VOID ChangeASM(HANDLE ProcessHandle);

	//�鿴�޸��ڴ�
	VOID ChangeMEM(HANDLE ProcessHandle);

	//�鿴�޸�ջ
	VOID ChangeStack(HANDLE ProcessHandle, HANDLE ThreadHandle);

	//��ȡ32λ��64λģ��
	void GetModule32and64(DWORD dwPid);
	
	//��ȡ����Լ����в������
	void GetModule();
};






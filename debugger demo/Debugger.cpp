#include <iostream>
#include <windows.h>
#include "Capstone.h"
#include "Debugger.h"
#include "BreakPoint.h"
using namespace std;


// �����Ե��Եķ�ʽ����һ������
bool Debugger::open(char const* FileName)
{
	// ��������ʱ��Ҫʹ�õĽṹ��
	PROCESS_INFORMATION ps = { 0 };
	STARTUPINFOA si = { sizeof(STARTUPINFO) };

	// ���ݴ�����ļ�[�Ե��Է�ʽ]����һ������
	BOOL isSuccess = CreateProcessA(FileName, NULL,
		NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS
		| CREATE_NEW_CONSOLE, NULL, NULL, &si, &ps);

	// �ж��Ƿ񴴽��ɹ���ʧ�ܷ���FALSE
	if (isSuccess == FALSE)
		return FALSE;

	// DEBUG_ONLY_THIS_PROCESS �� DEBUG_PROCESS �������
	// ���Ƿ���Ա����Գ��򴴽����ӽ���


	// �رս��̺��̵߳ľ��
	CloseHandle(ps.hThread);
	CloseHandle(ps.hProcess);

	// �ڴ������ԻỰʱִ�з��ر�������ĳ�ʼ��
	Capstone::Init();

	return true;
}

// �ȴ������¼�
void Debugger::run()
{
	// ���ڱ��������Ϣ�Ĵ�����
	DWORD Result = DBG_CONTINUE;

	while (TRUE)
	{
		WaitForDebugEvent(&DebugEvent, INFINITE);

		// �����쳣������λ�ô򿪾��
		OpenHandles();

		// ���ݵȴ����Ĳ�ͬ�����¼����д���
		switch (DebugEvent.dwDebugEventCode)
		{
			// ���յ��˲������쳣��Ϣ
		case EXCEPTION_DEBUG_EVENT:
			// ���쳣��Ϣ�Ľṹ�崫�뵽�����н��д���
			Result = OnExceptionHanlder();
			break;

			// ���յ��˽��̵Ĵ����¼�
		case CREATE_PROCESS_DEBUG_EVENT:
			StartAddress = DebugEvent.u.CreateProcessInfo.lpStartAddress;
			Result = DBG_CONTINUE;
			break;

			// �������Ҳ�����Ѵ���
		default:
			Result = DBG_CONTINUE;
			break;
		}

		// �ڴ������̺�ģ����ص�ʱ�򣬵�����Ϣ�б����� 
		// lpImageName �� fUnicode����������Ϣͨ��
		// ��û���õģ���Ӧ��ʹ����


		// �رվ��
		CloseHandles();

		// ���ߵ�����ϵͳ��ǰ������Ϣ�Ƿ񱻴���
		// �����е� PID �� TID �����ǵȴ�����
		// �¼�ʱ�������� ID��
		ContinueDebugEvent(
			DebugEvent.dwProcessId,
			DebugEvent.dwThreadId,
			// ��������ʾ�Ƿ���������¼�����
			// �������˷��� DBG_CONTINUE
			Result);
	}
}


// ר�����ڴ����쳣��Ϣ
DWORD Debugger::OnExceptionHanlder()
{
	// �쳣����ʱ�ĵ�ַ���쳣������
	DWORD ExceptionCode = DebugEvent.u.Exception.ExceptionRecord.ExceptionCode;
	LPVOID ExceptionAddress = DebugEvent.u.Exception.ExceptionRecord.ExceptionAddress;

	// ���ݲ�ͬ���쳣����ִ�в�ͬ�Ĳ���
	switch (ExceptionCode)
	{
		// �ڴ�ϵ��ʵ��
	case EXCEPTION_ACCESS_VIOLATION:
	{
		// �����������ڴ��ҳ����Ϊ ֻ��������ִ�С����ɶ�д
		// ��ҳ���Ե������� ��ҳ��СΪ��λ
		break;
	}

		// ����ϵ��ʵ��
	case EXCEPTION_BREAKPOINT:
	{
		// 1. �ж��ǲ���ϵͳ�ϵ�
		if (isSystemBreakPoint == TRUE)
		{
			// 2. �� OEP ��λ������һ������ϵ�
			BreakPoint::SetCcBreakPoint(ProcessHandle, StartAddress);

			// 3. ��һ�ξͲ���ϵͳ�ϵ���
			isSystemBreakPoint = FALSE;

			// 4. ���λ�ò�Ӧ�ý����û�������
			NeddCommand = FALSE;

			break;
		}
		
		// �޸���ǰ�Լ����õ�����ϵ�
		BreakPoint::FixCcBreakPoint(ProcessHandle, ThreadHandle, ExceptionAddress);
		break;
	}

		// Ӳ���ϵ��ʵ��
	case EXCEPTION_SINGLE_STEP:
	{
		// �޸�Ӳ���ϵ㣬�ó������ִ��
		BreakPoint::FixHdBreakPoint(ThreadHandle, ExceptionAddress);
		break;
	}
	}

	// �����Ҫ���²���������
	if (NeddCommand == TRUE)
	{
		Capstone::DisAsm(ProcessHandle, ExceptionAddress, 10);
		GetCommand();
	}
	
	// �����Ƿ���Ҫ����
	NeddCommand = TRUE;

	return DBG_CONTINUE;
}


// �����о��
VOID Debugger::OpenHandles()
{
	ThreadHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, DebugEvent.dwThreadId);
	ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, DebugEvent.dwProcessId);
}

// �ر����о��
VOID Debugger::CloseHandles()
{
	CloseHandle(ThreadHandle);
	CloseHandle(ProcessHandle);
}


// ��ȡ�û�������
VOID Debugger::GetCommand()
{
	// ���ڱ���ָ����ַ���
	CHAR Command[20] = { 0 };

	// ��ȡ�û�����
	while (cin >> Command)
	{
		// ���ݲ�ͬ������ִ�в�ͬ�Ĳ���
		if (!strcmp(Command, "t"))
		{
			// �����ϵ�
			BreakPoint::SetTfBreakPoint(ThreadHandle);
			break;
		}
		else if (!strcmp(Command, "g"))
		{
			// �����ܵ���һ���ϵ�λ�û�ִ�н���
			break;
		}
		else if (!strcmp(Command, "bp"))
		{
			DWORD Address = 0;
			cout << "����Ҫ���õĵ�ַ: ";
			scanf_s("%x", &Address);
			BreakPoint::SetCcBreakPoint(ProcessHandle, (LPVOID)Address);
			break;
		}
		else if (!strcmp(Command, "bhp"))
		{
			DWORD Address = 0;
			cout << "����Ҫ���õĵ�ַ: ";
			scanf_s("%x", &Address);
			BreakPoint::SetHdBreakPoint(ThreadHandle, (LPVOID)Address);
			break;
		}
		else
		{
			cout << "�����ָ�����" << endl;
		}
	}
}



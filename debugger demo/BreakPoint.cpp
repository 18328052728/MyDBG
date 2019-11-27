#include "BreakPoint.h"

// DR7�Ĵ����ṹ��
typedef struct _DBG_REG7 {
	unsigned L0 : 1; unsigned G0 : 1;
	unsigned L1 : 1; unsigned G1 : 1;
	unsigned L2 : 1; unsigned G2 : 1;
	unsigned L3 : 1; unsigned G3 : 1;
	unsigned LE : 1; unsigned GE : 1;
	unsigned : 6;// ��������Ч�ռ�
	unsigned RW0 : 2; unsigned LEN0 : 2;
	unsigned RW1 : 2; unsigned LEN1 : 2;
	unsigned RW2 : 2; unsigned LEN2 : 2;
	unsigned RW3 : 2; unsigned LEN3 : 2;
} R7, * PR7;


vector<ExceptionInfo> BreakPoint::BreakPointList;

// ����ʵ�ֵ����ϵ�: ͨ�� TF ��־λ
bool BreakPoint::SetTfBreakPoint(HANDLE ThreadHandle)
{
	// CPU �ڱ�־�Ĵ������ṩ��һ�� TF ��־λ���� CPU ִ��ָ���
	// �����У����������� TF ��־λʱ������ִͣ�У�������Ӳ���ϵ�
	// ���͵��쳣��֮�����á� TF ��־λ

	// 0. �ṩ�ṹ�屣���̻߳�������Ҫָ������Ҫ��ȡ�ļĴ�����
	CONTEXT Context = { CONTEXT_CONTROL };

	// 1. ��ȡ�̻߳���
	GetThreadContext(ThreadHandle, &Context);

	// 2. ͨ��λ�������õ� 8 λΪ 1
	Context.EFlags |= 0x100;

	// 3. ���޸ĵļĴ����������õ�Ŀ���߳�
	SetThreadContext(ThreadHandle, &Context);

	return TRUE;
}


// ����ʵ������ϵ�: ͨ�� int 3(0xCC) ָ��
bool BreakPoint::SetCcBreakPoint(HANDLE ProcessHandle, LPVOID Address)
{
	// ����ϵ��ԭ������޸�Ŀ������еġ���һ���ֽڡ�Ϊ
	// 0xCC���޸���ʱ����Ϊ int 3 ��������һ����������
	// ��������ָ�������һ��ָ���λ�ã���ô��Ҫ�� eip ִ
	// �м�����������ԭָ��

	SIZE_T Bytes = 0;
	DWORD OldProtect = 0;

	// 0. ����ϵ���Ϣ�Ľṹ��
	ExceptionInfo Int3Info = { CcFlag, Address };

	// 1. �޸��ڴ�ı�������
	VirtualProtectEx(ProcessHandle, Address, 1, PAGE_EXECUTE_READWRITE, &OldProtect);

	// 2. ��ȡ��ԭ�е����ݽ��б���
	ReadProcessMemory(ProcessHandle, Address, &Int3Info.u.OldOpcode, 1, &Bytes);

	// 3. �� 0xCC д��Ŀ��λ��
	WriteProcessMemory(ProcessHandle, Address, "\xCC", 1, &Bytes);

	// 4. ��ԭ�ڴ�ı�������
	VirtualProtectEx(ProcessHandle, Address, 1, OldProtect, &OldProtect);

	// 5. ����ϵ㵽�б�
	BreakPointList.push_back(Int3Info);

	return false;
}


// �޸�һ������ϵ�
bool BreakPoint::FixCcBreakPoint(HANDLE ProcessHandle, HANDLE ThreadHandle, LPVOID Address)
{
	// �жϵ�ǰ���µ��費��Ҫ�޸�
	for (size_t i = 0; i < BreakPointList.size(); ++i)
	{
		// ��һ������ϵ㲢�ҵ�ַ�ͱ���ĵ�ַ��ͬ����Ҫ�޸�
		if (BreakPointList[i].ExceptionFlag == CcFlag &&
			BreakPointList[i].ExceptionAddress == Address)
		{
			// 1. ��ȡ�̻߳�������Ϊ eip ָ����һ�������� -1
			CONTEXT Context = { CONTEXT_CONTROL };
			GetThreadContext(ThreadHandle, &Context);
			Context.Eip -= 1;
			SetThreadContext(ThreadHandle, &Context);

			// 2. ��ԭ������д��Ŀ��λ��
			DWORD OldProtect = 0, Bytes = 0;;
			VirtualProtectEx(ProcessHandle, Address, 1, PAGE_EXECUTE_READWRITE, &OldProtect);
			WriteProcessMemory(ProcessHandle, Address, &BreakPointList[i].u.OldOpcode, 1, &Bytes);
			VirtualProtectEx(ProcessHandle, Address, 1, OldProtect, &OldProtect);

			// 3. ����ϵ��ǲ������öϵ�,�費��Ҫ��ɾ��
			//  - ��Ҫɾ���� erase() 
			//  - ����Ҫɾ��������һ���Ƿ���Ч�ı�־λ
			BreakPointList.erase(BreakPointList.begin() + i);
			break;
		}
	}

	return true;
}

// ����ʵ��Ӳ���ϵ�: ͨ�� ���ԼĴ��� Dr0~Dr3 Dr7
bool BreakPoint::SetHdBreakPoint(HANDLE ThreadHandle, LPVOID Address, DWORD Type, DWORD Len)
{
	// �����������λ0����ô���ȱ���Ϊ0

	// ֧��Ӳ���ϵ�ļĴ����� 6 ���������� 4 �����ڱ����ַ
	// Ӳ���ϵ����������� 4 �����ٶ��ʧ����

	ExceptionInfo HdInfo = { HdFlag, Address };

	// ��ȡ�����ԼĴ���
	CONTEXT Context = { CONTEXT_DEBUG_REGISTERS };
	GetThreadContext(ThreadHandle, &Context);

	// ��ȡ Dr7 �ṹ�岢����
	PR7 Dr7 = (PR7)&Context.Dr7;

	// ͨ�� Dr7 �е�L(n) ֪����ǰ�ĵ��ԼĴ����Ƿ�ʹ��
	if (Dr7->L0 == FALSE)
	{
		// ����Ӳ���ϵ��Ƿ���Ч
		Dr7->L0 = TRUE;
		
		// ���öϵ������
		Dr7->RW0 = Type;

		// ���öϵ��ַ�Ķ��볤��
		Dr7->LEN0 = Len;

		// ���öϵ�ĵ�ַ
		Context.Dr0 = (DWORD)Address;
	}
	else if (Dr7->L1 == FALSE)
	{
		Dr7->L1 = TRUE;
		Dr7->RW1 = Type;
		Dr7->LEN1 = Len;
		Context.Dr1 = (DWORD)Address;
	}
	else if (Dr7->L2 == FALSE)
	{
		Dr7->L2 = TRUE;
		Dr7->RW2 = Type;
		Dr7->LEN2 = Len;
		Context.Dr2 = (DWORD)Address;
	}
	else if (Dr7->L3 == FALSE)
	{
		Dr7->L3 = TRUE;
		Dr7->RW3 = Type;
		Dr7->LEN3 = Len;
		Context.Dr3 = (DWORD)Address;
	}
	else
	{
		return false;
	}


	// ���޸ĸ��µ��߳�
	SetThreadContext(ThreadHandle, &Context);

	// ��ӵ��ϵ��б�
	BreakPointList.push_back(HdInfo);

	return true;
}


// �޸�һ������ϵ�
bool BreakPoint::FixHdBreakPoint(HANDLE ThreadHandle, LPVOID Address)
{
	// �޸��Ĺ����У�����Ҫ֪����ʲô�ϵ�
	for (size_t i = 0; i < BreakPointList.size(); ++i)
	{
		// �ж�����
		// ��һ������ϵ㲢�ҵ�ַ�ͱ���ĵ�ַ��ͬ����Ҫ�޸�
		if (BreakPointList[i].ExceptionFlag == HdFlag &&
			BreakPointList[i].ExceptionAddress == Address)
		{
			// ��ȡ�����ԼĴ���
			CONTEXT Context = { CONTEXT_DEBUG_REGISTERS };
			GetThreadContext(ThreadHandle, &Context);

			// ��ȡ Dr7 �Ĵ���
			PR7 Dr7 = (PR7)& Context.Dr7;

			// ���� Dr6 �ĵ� 4 λ֪����˭��������
			int index = Context.Dr6 & 0xF;

			// �������Ķϵ����ó���Ч��
			switch (index)
			{
			case 1: Dr7->L0 = 0; break;
			case 2:	Dr7->L0 = 0; break;
			case 4:	Dr7->L2 = 0; break;
			case 8:	Dr7->L3 = 0; break;
			}

			// ���޸ĸ��µ��߳�
			SetThreadContext(ThreadHandle, &Context);

			BreakPointList.erase(BreakPointList.begin() + i);
		}
	}

	return false;
}

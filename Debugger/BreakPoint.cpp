#include "pch.h"
#include "BreakPoint.h"

#include <iostream>

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
} R7, *PR7;

//��̬��Ա����ʹ��ǰ��Ҫ����
vector<ExceptionInfo> BreakPoint::BreakPointList;

int BreakPoint::TFAndHbp = 0;

// ����ʵ�ֵ����ϵ�: ͨ�� TF ��־λ
bool BreakPoint::SetTfBreakPoint(HANDLE ThreadHandle)
{
	// CPU �ڱ�־�Ĵ������ṩ��һ�� TF ��־λ���� CPU ִ��ָ���
	// �����У����������� TF ��־λʱ������ִͣ�У�������Ӳ���ϵ�
	// ���͵��쳣��֮�����á� TF ��־λ
	 
	//��TF���͵Ķϵ㲻��Ҫ�ָ�

	// 0. �ṩ�ṹ�屣���̻߳�������Ҫָ������Ҫ��ȡ�ļĴ��������Ӷ����EFlags�Ĵ���
	CONTEXT Context = { CONTEXT_CONTROL };

	// 1. ��ȡ�̻߳���
	GetThreadContext(ThreadHandle, &Context);

	// 2. ͨ��λ�������õ� 8 λΪ 1
	Context.EFlags |= 0x100;

	// 3. ���޸ĵļĴ����������õ�Ŀ���߳�
	SetThreadContext(ThreadHandle, &Context);
	
	//���ڱ�־TF�ϵ�Ĳ�������ñ�־Ϊ1
	TFAndHbp += 1;

	return TRUE;
}

bool BreakPoint::SetPassBreakPoint(HANDLE ProcessHandle, HANDLE ThreadHandle,DWORD NextCodeAddress)
{
	// 0. �ṩ�ṹ�屣���̻߳�������Ҫָ������Ҫ��ȡ�ļĴ��������Ӷ����EFlags�Ĵ���
	CONTEXT Context = { CONTEXT_CONTROL };

	// 1. ��ȡ�̻߳���
	GetThreadContext(ThreadHandle, &Context);

	//���ڱ����ȡ�������ݵĴ�С
	DWORD Bytes = 0;
	//���ڱ����ڴ�ԭ���ı�������
	DWORD OldProtect = 0;

	//���ڱ���һ���ֽڵ�opcode
	CHAR OPCode;

	// 1. �޸��ڴ�ı�������  �޸�һ���ڴ�ҳ��С���ڴ汣������
	VirtualProtectEx(ProcessHandle, (LPVOID)Context.Eip, 1, PAGE_EXECUTE_READWRITE, &OldProtect);

	// 2. ��ȡ��ԭ�е����ݽ��б��棬��ȡһ�ֽڵ�Դ���뵽�ϵ�ṹ����
	ReadProcessMemory(ProcessHandle, (LPVOID)Context.Eip, &OPCode, 1, &Bytes);

	// 4. ��ԭ�ڴ�ı�������
	VirtualProtectEx(ProcessHandle, (LPVOID)Context.Eip, 1, OldProtect, &OldProtect);


	if (OPCode == '\xF3' || OPCode == '\xF2' || OPCode == '\xFF'|| OPCode == '\xE8'|| OPCode == '\x9A')//����opcode��repָ���callָ���opcode
	{

		//�����ǰ����callָ���repָ������ڸ�ָ����һ��ָ���һ���ϵ�Ӷ��γɵ�������
		ExceptionInfo Int3Info = { CcFlag, (LPVOID)NextCodeAddress };
		Int3Info.EternalOrNot = 0;

		// 1. �޸��ڴ�ı�������  �޸�һ���ڴ�ҳ��С���ڴ汣������
		VirtualProtectEx(ProcessHandle, (LPVOID)NextCodeAddress, 1, PAGE_EXECUTE_READWRITE, &OldProtect);

		// 2. ��ȡ��ԭ�е����ݽ��б��棬��ȡһ�ֽڵ�Դ���뵽�ϵ�ṹ����
		ReadProcessMemory(ProcessHandle, (LPVOID)NextCodeAddress, &Int3Info.u.OldOpcode, 1, &Bytes);

		// 3. �� 0xCC д��Ŀ��λ��
		WriteProcessMemory(ProcessHandle, (LPVOID)NextCodeAddress, "\xCC", 1, &Bytes);

		// 4. ��ԭ�ڴ�ı�������
		VirtualProtectEx(ProcessHandle, (LPVOID)NextCodeAddress, 1, OldProtect, &OldProtect);

		// 5. ����ϵ㵽�б�
		BreakPointList.push_back(Int3Info);

		return TRUE;

	}
	else//�����ǰָ���callָ���repָ�ֱ������tf�ϵ㵥������
	{
		//tf�ϵ�
		// 2. ͨ��λ�������õ� 8 λΪ 1
		Context.EFlags |= 0x100;
		// 3. ���޸ĵļĴ����������õ�Ŀ���߳�
		SetThreadContext(ThreadHandle, &Context);
		//���ڱ�־TF�ϵ�Ĳ�������ñ�־Ϊ1
		TFAndHbp += 1;

		return TRUE;
	}


}


// ����ʵ������ϵ�: ͨ�� int 3(0xCC) ָ��
bool BreakPoint::SetCcBreakPoint(HANDLE ProcessHandle, LPVOID Address)
{
	// ����ϵ��ԭ������޸�Ŀ������еġ���һ���ֽڡ�Ϊ
	// 0xCC���޸���ʱ����Ϊ int 3 ��������һ����������
	// ��������ָ�������һ��ָ���λ�ã���ô��Ҫ�� eip ִ
	// �м�����������ԭָ��

	//���ڱ�����Զ�̽��̶�д���ֽ���
	SIZE_T Bytes = 0;
	//�����ڸ���Զ���̱߳�������ʱ�����ڴ�ԭ�еı�������
	DWORD OldProtect = 0;
	
	//�����ж϶ϵ��Ƿ�Ϊ����
	BOOL EternalFlag = 0;

	// 0. ����ϵ���Ϣ�Ľṹ�� �ϵ����� �ϵ��ַ
	ExceptionInfo Int3Info = { CcFlag, Address };


	std::cout << "�Ƿ�Ҫ������������ϵ�(1���� 0����)��";
	std::cin >> EternalFlag;
	
	if (EternalFlag == 1)
	{
		Int3Info.EternalOrNot = 1;
	}
	else if (EternalFlag == 0)
	{
		Int3Info.EternalOrNot = 0;
	}
	else 
	{
		std::cout << "��������������öϵ�ʧ��";
		return false;
	}

	
	// 1. �޸��ڴ�ı�������  �޸�һ���ڴ�ҳ��С���ڴ汣������
	VirtualProtectEx(ProcessHandle, Address, 1, PAGE_EXECUTE_READWRITE, &OldProtect);

	// 2. ��ȡ��ԭ�е����ݽ��б��棬��ȡһ�ֽڵ�Դ���뵽�ϵ�ṹ����
	ReadProcessMemory(ProcessHandle, Address, &Int3Info.u.OldOpcode, 1, &Bytes);

	// 3. �� 0xCC д��Ŀ��λ��
	WriteProcessMemory(ProcessHandle, Address, "\xCC", 1, &Bytes);

	// 4. ��ԭ�ڴ�ı�������
	VirtualProtectEx(ProcessHandle, Address, 1, OldProtect, &OldProtect);

	// 5. ����ϵ㵽�б�
	BreakPointList.push_back(Int3Info);

	//�ú�����return false ���ܴ������⣬��Ϊ֮��Ҫ�����޸�
	return false;
}


// �޸�һ������ϵ㣬��Ϊint3����������ϵ㣬Ϊ���ٴ�ִ�д���ָ���Ҫ��eip��С��������Ҫ����쳣�߳��е�eip����Ҫ�߳̾����Ϊ����
bool BreakPoint::FixCcBreakPoint(HANDLE ProcessHandle, HANDLE ThreadHandle, LPVOID Address)//��Ҫ�ָ��ϵ�ĵ�ַ
{
	//�����ж��Ƿ�int3�޸��ɹ���
	BOOL WriteSuccess=FALSE;

	// �жϵ�ǰ���µ��費��Ҫ�޸� ����������ϵ�Ķ�̬����
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
				
				WriteSuccess = WriteProcessMemory(ProcessHandle, Address, &BreakPointList[i].u.OldOpcode, 1, &Bytes);
				
				VirtualProtectEx(ProcessHandle, Address, 1, OldProtect, &OldProtect);

				// 3. ����ϵ��ǲ������öϵ�,�費��Ҫ��ɾ��
				//  - ��Ҫɾ���� erase() 
				//  - ����Ҫɾ��������һ���Ƿ���Ч�ı�־λ
				if (BreakPointList[i].EternalOrNot == 0)//�������������,��ɾ���ϵ㣬��������öϵ���ContinueDebugEventִ�����֮��				
				{                                       //��WaitForDebugEvent֮�����öϵ㸴ԭ����֤��һ��ִ�е���ʱ���ܶ�ס
					BreakPointList.erase(BreakPointList.begin() + i);
				}

				break;	
		}	
	}
	return WriteSuccess;
}

// ����ʵ��Ӳ���ϵ�: ͨ�� ���ԼĴ��� Dr0~Dr3 Dr7
//����dr7�еı�־λ rw0~rw3 0����ʾִ�жϵ�   1����ʾд�ϵ�  3����ʾ��д�ϵ㣬��ȡָ������ִ�г���
//len0~3 0��1�ֽڳ��ȣ�ִ�жϵ�ֻ����1�ֽڳ��ȣ� 1��2�ֽڳ��ȣ��ϵ��ַ����Ϊ2�ı��������϶��루��Ҫ�ں����ڲ����ϵ��ַ���룩
//2��8�ֽڳ���δ���峤��  3�����ֽڳ��ȣ��ϵ��ַ������4�ı������϶��루��Ҫ�ں����ڲ����ϵ��ַ���룩
bool BreakPoint::SetHdBreakPoint(HANDLE ThreadHandle, LPVOID Address, DWORD Type, DWORD Len)//�쳣�̡߳��¶ϵ�ַ���ϵ����͡��ϵ㳤��
{       
	// �����������λ0����ô���ȱ���Ϊ0

	// ֧��Ӳ���ϵ�ļĴ����� 6 ���������� 4 �����ڱ����ַ
	// Ӳ���ϵ����������� 4 �����ٶ��ʧ����
	
	//�����ж϶ϵ��Ƿ�Ϊ����
	BOOL EternalFlag = 0;
	//���ڱ�־����Ӳ���ϵ���dr0~3�ĸ��Ĵ������õ�
	int Dr = 0;
	
	if (Len == 1)//2�ֽڶ�������
	{
		Address = (LPVOID)((DWORD)Address - (DWORD)Address % 2);
	}
	else if (Len == 3)//4�ֽڶ�������
	{
		Address = (LPVOID)((DWORD)Address - (DWORD)Address % 4);
	}
	else if (Len > 3)
	{
		std::cout << "�ڴ��������������󣬶ϵ�����ʧ��";
		return false;
	}


	//����ϵ�ṹ��
	ExceptionInfo HdInfo = { HdFlag, Address };

	//����Ӳ���ϵ����ͺͶ�������
	HdInfo.HdBpType = Type;

	HdInfo.HdBpLen = Len;



	std::cout << "�Ƿ�Ҫ��������Ӳ���ϵ�(1���� 0����)��";
	std::cin >> EternalFlag;


	if (EternalFlag == 1)
	{
		HdInfo.EternalOrNot = 1;

		std::cout << "����������Ӳ���ϵ�Ҫ�������ĸ��Ĵ���(DR0:0 DR1:1 DR2:2 DR3:3)��";
		std::cin >> Dr;
		if (Dr != 1 && Dr != 0 && Dr != 2 && Dr != 3)
		{
			std::cout << "�Ĵ������������󣬶ϵ�����ʧ��";
			return false;
		}
		else 
		{
			HdInfo.u.DRnumber = Dr;
		}
	}
	else if (EternalFlag == 0)
	{
		HdInfo.EternalOrNot = 0;
	}
	else
	{
		std::cout << "��������������öϵ�ʧ��";
		return false;
	}


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
		//�ϵ����� 0��ִ��ʱ�ж�   1��д����ʱ�ж�  3��дʱ�жϣ���ȡָ������ִ�г���
		Dr7->RW0 = Type;

		// ���öϵ��ַ�Ķ��볤�� 
		//0��һ�ֽڳ��ȶ���  1��2�ֽڳ��ȶ���ϵ��ַ����Ϊ2�ı���  2��8�ֽڳ���δ���峤��  3��4�ֽڳ��ȣ��ϵ��ַ������4�ı���
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
		cout << "Ӳ���ϵ������ﵽ���ޣ�Ӳ���ϵ�����ʧ��\n";
		return false;
	}

	// ���޸ĸ��µ��߳�
	SetThreadContext(ThreadHandle, &Context);

	// ��ӵ��ϵ��б�
	BreakPointList.push_back(HdInfo);

	//��־λ�ٴ�����1����־Ӳ���ϵ������
	TFAndHbp += 1;

	return true;
}


// �޸�һ��Ӳ���ϵ�
bool BreakPoint::FixHdBreakPoint(HANDLE ProcessHandle,HANDLE ThreadHandle, LPVOID Address,int*IsOrNot)
{
	DWORD OldProtect;
	int TfForMembp = 0;//��ʶ�Ƿ������ڴ�ϵ�Ļָ�
	int HdBp = 0;//��ʶ��ǰ�쳣�Ƿ���Ϊ�˴���Ӳ���ϵ�


	for (size_t i = 0; i < BreakPointList.size(); ++i)
	{
		if (BreakPointList[i].ExceptionFlag == MemFlag)//������������ڴ�ϵ㣬˵����ʱ�쳣���޸��ڴ��쳣��tf�ϵ�����ģ���Ҫ�����ｫ�ڴ�ϵ��޸�
		{
			if (BreakPointList[i].TypeOfMemBp == 0)//ִ�жϵ�
			{
				//VirtualProtectEx(ProcessHandle, Address, 1, PAGE_READWRITE, &OldProtect);
				VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, PAGE_NOACCESS, &OldProtect);
				//PAGE_READWRITE 
			}
			else if (BreakPointList[i].TypeOfMemBp == 1)	//д��ϵ㣺PAGE_EXECUTE_READ�ܹ�ִ�л��������д����������д�ϵ�
			{
				VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, PAGE_EXECUTE_READ, &OldProtect);
			}
			else if (BreakPointList[i].TypeOfMemBp == 2)//��ȡ�ϵ�
			{
				VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, PAGE_WRITECOPY&&PAGE_EXECUTE, &OldProtect);
			}
			TfForMembp = 1;
		
		}
	}


	//�����־λΪ2��˵����tf�ϵ�֮ǰ�Ѿ�������Ӳ���ϵ㣬���β���Ҫ�޸���Ӳ���ϵ㣬ֱ���������õ�Ӳ���ϵ�
	if (TFAndHbp == 2) 
	{
		TFAndHbp-=1;
		return TRUE;
	}
	if (TFAndHbp == 1)//��־λΪ1˵����ǰ�����ϵ����һ����Ӳ���ϵ���Ҫ�޸�
	{
		TFAndHbp = 0;
	}

	//��ΪӲ���ϵ�ֻ�����ǵ������¸������Գ���ģ�����TF�ϵ�͵��ԼĴ����ϵ㶼�ᴥ������ִ���쳣��
	//������ִ���쳣������TF�ϵ�ʱ������ִ�жϵ��޸���������Ϊtfλ�ڴ����쳣֮��ͻ�ָ�ԭ����ֵ
	//���� FixSuccess����Ӧ������Ϊtrue
	BOOL FixSuccess = TRUE;
	

	// �޸��Ĺ����У�����Ҫ֪����ʲô�ϵ�
	for (size_t i = 0; i < BreakPointList.size(); ++i)
	{
		//if (BreakPointList[i].ExceptionFlag == MemFlag)//������������ڴ�ϵ㣬˵����ʱ�쳣���޸��ڴ��쳣��tf�ϵ�����ģ���Ҫ�����ｫ�ڴ�ϵ��޸�
		//{
		//	if (BreakPointList[i].TypeOfMemBp == 0)//ִ�жϵ�
		//	{
		//		//VirtualProtectEx(ProcessHandle, Address, 1, PAGE_READWRITE, &OldProtect);
		//		VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, PAGE_NOACCESS, &OldProtect);
		//		//PAGE_READWRITE 
		//	}
		//	else if (BreakPointList[i].TypeOfMemBp == 1)	//д��ϵ㣺PAGE_EXECUTE_READ�ܹ�ִ�л��������д����������д�ϵ�
		//	{
		//		VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, PAGE_EXECUTE_READ, &OldProtect);
		//	}
		//	else if (BreakPointList[i].TypeOfMemBp == 2)//��ȡ�ϵ�
		//	{
		//		VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, PAGE_WRITECOPY&&PAGE_EXECUTE, &OldProtect);
		//	}
		//}

		// �ж�����
		// ��һ��Ӳ���ϵ㲢�ҵ�ַ�ͱ���ĵ�ַ��ͬ����Ҫ�޸�
		if (BreakPointList[i].ExceptionFlag == HdFlag &&
			BreakPointList[i].ExceptionAddress == Address)
		{
			// ��ȡ�����ԼĴ���
			CONTEXT Context = { CONTEXT_DEBUG_REGISTERS };
			GetThreadContext(ThreadHandle, &Context);

			// ��ȡ Dr7 �Ĵ���
			PR7 Dr7 = (PR7)& Context.Dr7;


			if (BreakPointList[i].EternalOrNot == 0)
			{
				//����ʹ��dr6�жϵ�ǰӲ���ϵ���dr0~3���ĸ����µĺ���������⣬

				// ���� Dr6 �ĵ� 4 λ֪����˭��������
				int index = Context.Dr6 & 0xF;

				// �������Ķϵ����ó���Ч��
				switch (index)
				{
				case 1: Dr7->L0 = 0; break;
				case 2:	Dr7->L1 = 0; break;
				case 4:	Dr7->L2 = 0; break;
				case 8:	Dr7->L3 = 0; break;
				}
			}
			else if (BreakPointList[i].EternalOrNot == 1)
			{
				switch (BreakPointList[i].u.DRnumber) //��������Ӳ���ϵ��б��������Ӳ���ϵ��Ӧ�ļĴ�����ţ�����Ӧ����Ӳ���ϵ�ָ�
				{
				case 0: Dr7->L0 = 0; break;
				case 1:	Dr7->L1 = 0; break;
				case 2:	Dr7->L2 = 0; break;
				case 3:	Dr7->L3 = 0; break;
				}
			}


			// ���޸ĸ��µ��߳�
			FixSuccess=SetThreadContext(ThreadHandle, &Context);

			if (BreakPointList[i].EternalOrNot == 0)//����öϵ㲻��Ӳ���ϵ㣬����Ӷ�̬������ɾ��
			{
				BreakPointList.erase(BreakPointList.begin() + i);
			}
			HdBp = 1;
		}
	}


	if (HdBp == 0 && TfForMembp == 0)//�������ֻ�Ǵ���tf�ϵ�
	{
		*IsOrNot = 1;//��Ҫ�����û�����
	}
	if (HdBp == 1 && TfForMembp == 0)//�������ֻ�Ǵ���Ӳ���ϵ�
	{
		*IsOrNot = 1;//��Ҫ�����û�����
	}
	if (HdBp == 1 && TfForMembp == 1)//��������ȴ���Ӳ���ϵ㣬Ҳ�����ڴ�ϵ㼰�丽����tf�ϵ�
	{
		*IsOrNot = 1;//��Ҫ�����û����룬����Ӳ���ϵ�ϲ���
	}
	if (HdBp == 0 && TfForMembp == 1)//�������ֻ�Ǵ����ڴ�ϵ㼰�丽����tf�ϵ㣬��������Ӳ���ϵ�
	{
		*IsOrNot = 0;//����Ҫ�����û�����
	}


	return FixSuccess;
}


//���������������ϵ�
bool BreakPoint::ReSetCcBreakPointOfEnternal(HANDLE ProcessHandle,HANDLE ThreadHandle)
{

	//���ڱ�����Զ�̽��̶�д���ֽ���
	SIZE_T Bytes = 0;
	//�����ڸ���Զ���̱߳�������ʱ�����ڴ�ԭ�еı�������
	DWORD OldProtect = 0;

	for (size_t i = 0; i < BreakPointList.size(); ++i)
	{
		// ��һ������ϵ㲢�ҵ�ַ�ͱ���ĵ�ַ��ͬ����Ҫ�޸�
		if (BreakPointList[i].ExceptionFlag == CcFlag &&
			BreakPointList[i].EternalOrNot == 1)
		{
			//������������ϵ�,���ﲻ��ֱ�ӵ���SetCcBreakPoint����������������ѭ��
			//SetCcBreakPoint(ProcessHandle, BreakPointList[i].ExceptionAddress);
		
			// 1. �޸��ڴ�ı�������  �޸�һ���ֽڵ��ڴ汣������
			VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, PAGE_EXECUTE_READWRITE, &OldProtect);

			//���ﲻ��Ҫ���ɵ�opcode������������Ϊ֮ǰ�¶ϵ��ʱ���Ѿ��������

			// 3. �� 0xCC д��Ŀ��λ��
			WriteProcessMemory(ProcessHandle, BreakPointList[i].ExceptionAddress, "\xCC", 1, &Bytes);

			// 4. ��ԭ�ڴ�ı�������
			VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, OldProtect, &OldProtect);	
		}
	
		//�����Ӳ�����öϵ�
		if (BreakPointList[i].ExceptionFlag == HdFlag &&
			BreakPointList[i].EternalOrNot == 1)
		{
			// ��ȡ�����ԼĴ���
			CONTEXT Context = { CONTEXT_DEBUG_REGISTERS };
			GetThreadContext(ThreadHandle, &Context);

			// ��ȡ Dr7 �Ĵ���
			PR7 Dr7 = (PR7)& Context.Dr7;

			//��Ӧ������Ӳ���ϵ���������Ϊ��Ч
			switch (BreakPointList[i].u.DRnumber) //��������Ӳ���ϵ��б��������Ӳ���ϵ��Ӧ�ļĴ�����ţ�����Ӧ����Ӳ���ϵ�ָ�
			{
			case 0: 
				Dr7->L0 = 1;
				Context.Dr0 = (DWORD)BreakPointList[i].ExceptionAddress; 
				Dr7->RW0 = BreakPointList[i].HdBpType; 
				Dr7->LEN0 = BreakPointList[i].HdBpLen;
				break;
			case 1:	
				Dr7->L1 = 1; 
				Context.Dr1 = (DWORD)BreakPointList[i].ExceptionAddress; 
				Dr7->RW1 = BreakPointList[i].HdBpType;
				Dr7->LEN1 = BreakPointList[i].HdBpLen;
				break;
			case 2:	
				Dr7->L2 = 1; 
				Context.Dr2 = (DWORD)BreakPointList[i].ExceptionAddress; 
				Dr7->RW2 = BreakPointList[i].HdBpType;
				Dr7->LEN2 = BreakPointList[i].HdBpLen;
				break;
			case 3:	
				Dr7->L3 = 1; 
				Context.Dr3 = (DWORD)BreakPointList[i].ExceptionAddress; 
				Dr7->RW3 = BreakPointList[i].HdBpType;
				Dr7->LEN3 = BreakPointList[i].HdBpLen;
				break;
			}
			// ���޸ĸ��µ��߳�
			SetThreadContext(ThreadHandle, &Context);
		
		/*		// ���öϵ������  
		//�ϵ����� 0��ִ��ʱ�ж�   1��д����ʱ�ж�  3��дʱ�жϣ���ȡָ������ִ�г���
		Dr7->RW0 = Type;

		// ���öϵ��ַ�Ķ��볤�� 
		//0��һ�ֽڳ��ȶ���  1��2�ֽڳ��ȶ���ϵ��ַ����Ϊ2�ı���  2��8�ֽڳ���δ���峤��  3��4�ֽڳ��ȣ��ϵ��ַ������4�ı���
		Dr7->LEN0 = Len;*/		
		}


		//if (BreakPointList[i].ExceptionFlag == MemFlag &&
		//	BreakPointList[i].EternalOrNot == 1)
		//{
		//	//����������ڴ�ϵ���Ҫ���ڴ汣�������ٴ����ã��ѻָ��ڴ�ϵ�
		//	if (BreakPointList[i].TypeOfMemBp == 0)//ִ�жϵ�
		//	{
		//		//VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, PAGE_READWRITE, &OldProtect);

		//		VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, PAGE_NOACCESS, &OldProtect);
		//	}
		//	else if (BreakPointList[i].TypeOfMemBp ==1)	//д��ϵ㣺PAGE_EXECUTE_READ�ܹ�ִ�л��������д����������д�ϵ�
		//	{
		//		VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, PAGE_EXECUTE_READ, &OldProtect);
		//	}
		//	else if (BreakPointList[i].TypeOfMemBp == 2)//��ȡ�ϵ�
		//	{
		//		VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, PAGE_WRITECOPY&&PAGE_EXECUTE, &OldProtect);
		//	}	
		//}
	}
	//���� ��Ϊ SetCcBreakPoint������ִ�н�����ֻ�ܷ���false�����Ա��������޷��ж��Ƿ����öϵ��޸��ɹ���Ĭ�Ϸ���true��������Ժ���
	return true;
}


//�����ڴ�ϵ�
bool BreakPoint::SetMemBreakPoint(HANDLE ProcessHandle, LPVOID Address)
{
	int TypeOfMemBp = 0;
	
	//���ڱ����ڴ�ϵ�
	ExceptionInfo MemInfo = { MemFlag, Address };

	//���ڱ�����Զ�̽��̶�д���ֽ���
	SIZE_T Bytes = 0;
	//�����ڸ���Զ���̱߳�������ʱ�����ڴ�ԭ�еı�������
	DWORD OldProtect = 0;

	//����ڴ�ϵ��ַ��Ӧ�ڴ�ҳ���׵�ַ
	DWORD HandOfMemBpPage = (DWORD)Address & 0xFFFFF000;
	
	//����¶ϵ�ĵ�ַλ�ڸ��ڴ�ҳͷ�����ֽ����ڣ���ı���ڴ�ҳ����һ���ڴ�ҳ�ı������ԣ�
	//���ڿ��ܸı���ڴ�ҳ��һ���ڴ�ҳ����������ս��һ������������
	if ((DWORD)Address - HandOfMemBpPage <= 2)
	{
		MemInfo.HeaderOFMemPage = HandOfMemBpPage - 0x1000;//����һ���ڴ�ҳ���׵�ַ��������
	}
	else
	{
		MemInfo.HeaderOFMemPage = HandOfMemBpPage;//���浱ǰ�ڴ�ҳ���׵�ַ
	}

	BOOL Enternal = 0;
	std::cout << "�Ƿ���Ҫ�������öϵ㣨�ǣ�1 ��0��";
	std::cin >> Enternal;

	if (Enternal)//��������öϵ�
	{
		MemInfo.EternalOrNot = 1;
	}
	else//����������öϵ�
	{
		MemInfo.EternalOrNot = 0;
	}
	

	//PAGE_NOACCESS ���ɶ�����д������ִ��
	
	//PAGE_EXECUTE_READWRITE �ɶ���д��ִ��
	
	//PAGE_READWRITE �ɶ���д  ����ִ�жϵ�
	//PAGE_EXECUTE ��ִ�� ����д�ϵ�

    //PAGE_READONLY ֻ������������д�ϵ�

	std::cout << "�����ڴ�ϵ����� ��ִ�жϵ㣺0  д��ϵ㣺1  ��ȡ�ϵ㣺2��";

	std::cin >> TypeOfMemBp;

	if (TypeOfMemBp != 0 && TypeOfMemBp != 1 && TypeOfMemBp != 2)
	{
		std::cout << "�ڴ�ϵ�����������󣬶ϵ�����ʧ��";
		return FALSE;
	}

	//���ڴ�ϵ����ͱ�������
	MemInfo.TypeOfMemBp= TypeOfMemBp;

	if (TypeOfMemBp == 0)//ִ�жϵ�
	{
		//VirtualProtectEx(ProcessHandle, Address, 1, PAGE_READWRITE, &OldProtect);
		VirtualProtectEx(ProcessHandle, Address, 1, PAGE_NOACCESS, &OldProtect);
	//PAGE_READWRITE 
	}
	else if (TypeOfMemBp == 1)	//д��ϵ㣺PAGE_EXECUTE_READ�ܹ�ִ�л��������д����������д�ϵ�
	{
		VirtualProtectEx(ProcessHandle, Address, 1, PAGE_EXECUTE_READ, &OldProtect);
	}
	else if (TypeOfMemBp == 2)//��ȡ�ϵ�
	{
		VirtualProtectEx(ProcessHandle, Address, 1, PAGE_WRITECOPY&&PAGE_EXECUTE, &OldProtect);
	}

	//VirtualProtectEx(ProcessHandle, Address, 1, PAGE_READWRITE, &OldProtect);
	
	// 1. �޸��ڴ�ı�������Ϊ���ɶ�����д���ɷ��ʣ��Դ���Ϊ�ϵ�

	//��ԭ�����ڴ汣�����Ա�������
	MemInfo.u.ProtectOfOld = OldProtect;

	// 4. ��ԭ�ڴ�ı�������
	//VirtualProtectEx(ProcessHandle, Address, 1, OldProtect, &OldProtect);

	// 5. ����ϵ㵽�б�
	BreakPointList.push_back(MemInfo);

	//VirtualProtect
	return TRUE;
}

//�޸��ڴ�ϵ�
bool BreakPoint::FixMemBreakPoint(HANDLE ProcessHandle, HANDLE ThreadHandle ,LPVOID Address,int*IsOrNot)
{
	//���ڷ����ڴ�ԭ�б�������
	DWORD Protect = 0;
	//�Ƚ�IsOrNotĬ������Ϊ0�������еĲ������ڴ�ϵ�ĵ�ַ������Ҫ�û�����
	*IsOrNot = 0;

	for (size_t i = 0; i < BreakPointList.size(); ++i)
	{
		if (BreakPointList[i].ExceptionFlag == MemFlag)
		{
			//����ڴ�ϵ��λ���ǵ������ڴ�ϵ��λ�ã������ֱ�ӽ��ڴ����Ի�ԭ���޸��ϵ㣬Ȼ������û�����
			if (Address == BreakPointList[i].ExceptionAddress)
			{
				//�ָ��ϵ��ڴ�鴦ԭ�е��ڴ汣������
				VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, BreakPointList[i].u.ProtectOfOld, &Protect);
				
				//��Ҫ�����û����룬�����Զ���
				*IsOrNot = 1;

				//����������öϵ�
				//if (BreakPointList[i].EternalOrNot == 0)
				//{
					//��ԭ�����ڴ�ϵ�ɾ��
					BreakPointList.erase(BreakPointList.begin() + i);
				//}
				
				//�����Ѵ���
				return TRUE;
			}

			if ((DWORD)BreakPointList[i].ExceptionAddress - BreakPointList[i].HeaderOFMemPage <= 2)//����ϵ��λ�������ڴ�ҳ�ϱ߽�2�ֽ����ڣ���˵��֮ǰ�ڴ�ϵ�ñ��������ڴ�ҳ�Ķ�д����
			{
				if ((DWORD)Address >= (BreakPointList[i].HeaderOFMemPage - 0x1000) &&
					(DWORD)Address <= (BreakPointList[i].HeaderOFMemPage + 0x1000))//�����ڴ��쳣��λ�������˱��ı���ڴ����Ե��ڴ�ҳ�еĵ�ַ
				{

					//�ָ��ϵ��ڴ�鴦ԭ�е��ڴ汣������
					VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, BreakPointList[i].u.ProtectOfOld, &Protect);

					//�ڴ����쳣�ĵط�����һ��tf�ϵ�

					//0. �ṩ�ṹ�屣���̻߳�������Ҫָ������Ҫ��ȡ�ļĴ��������Ӷ����EFlags�Ĵ���
					CONTEXT Context = { CONTEXT_CONTROL };

					// 1. ��ȡ�̻߳���
					GetThreadContext(ThreadHandle, &Context);

					// 2. ͨ��λ�������õ� 8 λΪ 1
					Context.EFlags |= 0x100;

					// 3. ���޸ĵļĴ����������õ�Ŀ���߳�
					SetThreadContext(ThreadHandle, &Context);

					//֮�󴥷�tf�ϵ�ʱ��Ӧ�ý��ڴ������������û������Ӷ����ڴ�ϵ������������
					//����Ҫ��tf�ϵ�Ĵ��������жϵ�ǰ���µ�tf�ϵ��Ƿ��������ڴ�ϵ㵼�µ�



					//���ڱ�־TF�ϵ�Ĳ�������ñ�־Ϊ1
					//�����Ƿ�Ҫ��1��Ҫ��ȶ
					//TFAndHbp += 1;

					//��BreakPointList[i].ExceptionAddress���ĵ�ַ����һ������ϵ㣬��������ڸô���ס
					////���ڱ�����Զ�̽��̶�д���ֽ���
					//SIZE_T Bytes = 0;

					////�����ڸ���Զ���̱߳�������ʱ�����ڴ�ԭ�еı�������
					//DWORD OldProtect = 0;

					//// 0. ����ϵ���Ϣ�Ľṹ�� �ϵ����� �ϵ��ַ
					//ExceptionInfo Int3Info = { CcFlag, BreakPointList[i].ExceptionAddress };

					////���øöϵ�Ϊ�����öϵ㣨��ʱ�����ڿ��Ըģ�

					//Int3Info.EternalOrNot = 0;

					//// 1. �޸��ڴ�ı�������  �޸�һ���ڴ�ҳ��С���ڴ汣������
					//VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, PAGE_EXECUTE_READWRITE, &OldProtect);

					//// 2. ��ȡ��ԭ�е����ݽ��б��棬��ȡһ�ֽڵ�Դ���뵽�ϵ�ṹ����
					//ReadProcessMemory(ProcessHandle, BreakPointList[i].ExceptionAddress, &Int3Info.u.OldOpcode, 1, &Bytes);

					//// 3. �� 0xCC д��Ŀ��λ��
					//WriteProcessMemory(ProcessHandle, BreakPointList[i].ExceptionAddress, "\xCC", 1, &Bytes);

					//// 4. ��ԭ�ڴ�ı�������
					//VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, OldProtect, &OldProtect);

					//// 5. ���������ӵ�int3�ϵ㵽�б�
					//BreakPointList.push_back(Int3Info);

					////����������öϵ�
					//if (BreakPointList[i].EternalOrNot == 0)
					//{
					//	//��ԭ�����ڴ�ϵ�ɾ��
					//	BreakPointList.erase(BreakPointList.begin() + i);
					//}
					
					return TRUE;
				}
			}
			else if ((BreakPointList[i].HeaderOFMemPage + 0x1000) - (DWORD)BreakPointList[i].ExceptionAddress <= 2)//����ڴ�ϵ�������ڴ��±߽�2���ֽ�����
			{
				if ((DWORD)Address >= (BreakPointList[i].HeaderOFMemPage) &&
					(DWORD)Address <= (BreakPointList[i].HeaderOFMemPage + 0x2000))//�����ڴ��쳣��λ�������˱��ı���ڴ����Ե��ڴ�ҳ�еĵ�ַ
				{

					//�ָ��ϵ��ڴ�鴦ԭ�е��ڴ汣������
					VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, BreakPointList[i].u.ProtectOfOld, &Protect);
					
					///////////////////////////////////////////////
					
					//0. �ṩ�ṹ�屣���̻߳�������Ҫָ������Ҫ��ȡ�ļĴ��������Ӷ����EFlags�Ĵ���
					CONTEXT Context = { CONTEXT_CONTROL };

					// 1. ��ȡ�̻߳���
					GetThreadContext(ThreadHandle, &Context);

					// 2. ͨ��λ�������õ� 8 λΪ 1
					Context.EFlags |= 0x100;

					// 3. ���޸ĵļĴ����������õ�Ŀ���߳�
					SetThreadContext(ThreadHandle, &Context);

					//���ڱ�־TF�ϵ�Ĳ�������ñ�־Ϊ1
					//TFAndHbp += 1;

					///////////////////////////////////////////////

					////��BreakPointList[i].ExceptionAddress���ĵ�ַ����һ������ϵ㣬��������ڸô���ס

					////���ڱ�����Զ�̽��̶�д���ֽ���
					//SIZE_T Bytes = 0;

					////�����ڸ���Զ���̱߳�������ʱ�����ڴ�ԭ�еı�������
					//DWORD OldProtect = 0;

					//// 0. ����ϵ���Ϣ�Ľṹ�� �ϵ����� �ϵ��ַ
					//ExceptionInfo Int3Info = { CcFlag, BreakPointList[i].ExceptionAddress };

					////���øöϵ�Ϊ�����öϵ㣨��ʱ�����ڿ��Ըģ�
					//Int3Info.EternalOrNot = 0;


					//// 1. �޸��ڴ�ı�������  �޸�һ���ڴ�ҳ��С���ڴ汣������
					//VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, PAGE_EXECUTE_READWRITE, &OldProtect);

					//// 2. ��ȡ��ԭ�е����ݽ��б��棬��ȡһ�ֽڵ�Դ���뵽�ϵ�ṹ����
					//ReadProcessMemory(ProcessHandle, BreakPointList[i].ExceptionAddress, &Int3Info.u.OldOpcode, 1, &Bytes);

					//// 3. �� 0xCC д��Ŀ��λ��
					//WriteProcessMemory(ProcessHandle, BreakPointList[i].ExceptionAddress, "\xCC", 1, &Bytes);

					//// 4. ��ԭ�ڴ�ı�������
					//VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, OldProtect, &OldProtect);

					//// 5. ���������ӵ�int3�ϵ㵽�б�
					//BreakPointList.push_back(Int3Info);

					////����������öϵ�
					//if (BreakPointList[i].EternalOrNot == 0)
					//{
					//	//��ԭ�����ڴ�ϵ�ɾ��
					//	BreakPointList.erase(BreakPointList.begin() + i);
					//}				
					//��������ڴ��쳣�Ķϵ�պ�ʱ�ڴ�ϵ��¶ϵ�λ�ã�����Ҫ�����û����룬�����ֱ��������
						return TRUE;	
				}
			}
			else//����ڴ�ϵ�������ڴ�ҳ�ϡ��±߽�2���ֽ�����
			{
				if ((DWORD)Address >= (BreakPointList[i].HeaderOFMemPage) &&
					(DWORD)Address <= (BreakPointList[i].HeaderOFMemPage + 0x1000))//�����ڴ��쳣��λ�������˱��ı���ڴ����Ե��ڴ�ҳ�еĵ�ַ
				{

					//�ָ��ϵ��ڴ�鴦ԭ�е��ڴ汣������
					VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, BreakPointList[i].u.ProtectOfOld, &Protect);

					///////////////////////////////////////////////

					//0. �ṩ�ṹ�屣���̻߳�������Ҫָ������Ҫ��ȡ�ļĴ��������Ӷ����EFlags�Ĵ���
					CONTEXT Context = { CONTEXT_CONTROL };

					// 1. ��ȡ�̻߳���
					GetThreadContext(ThreadHandle, &Context);

					// 2. ͨ��λ�������õ� 8 λΪ 1
					Context.EFlags |= 0x100;

					// 3. ���޸ĵļĴ����������õ�Ŀ���߳�
					SetThreadContext(ThreadHandle, &Context);

					//���ڱ�־TF�ϵ�Ĳ�������ñ�־Ϊ1
					//TFAndHbp += 1;

					///////////////////////////////////////////////


					////��BreakPointList[i].ExceptionAddress���ĵ�ַ����һ������ϵ㣬��������ڸô���ס

					////���ڱ�����Զ�̽��̶�д���ֽ���
					//SIZE_T Bytes = 0;

					////�����ڸ���Զ���̱߳�������ʱ�����ڴ�ԭ�еı�������
					//DWORD OldProtect = 0;

					//// 0. ����ϵ���Ϣ�Ľṹ�� �ϵ����� �ϵ��ַ
					//ExceptionInfo Int3Info = { CcFlag, BreakPointList[i].ExceptionAddress };

					////���øöϵ�Ϊ�����öϵ㣨��ʱ�����ڿ��Ըģ�
					//Int3Info.EternalOrNot = 0;


					//// 1. �޸��ڴ�ı�������  �޸�һ���ڴ�ҳ��С���ڴ汣������
					//VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, PAGE_EXECUTE_READWRITE, &OldProtect);

					//// 2. ��ȡ��ԭ�е����ݽ��б��棬��ȡһ�ֽڵ�Դ���뵽�ϵ�ṹ����
					//ReadProcessMemory(ProcessHandle, BreakPointList[i].ExceptionAddress, &Int3Info.u.OldOpcode, 1, &Bytes);

					//// 3. �� 0xCC д��Ŀ��λ��
					//WriteProcessMemory(ProcessHandle, BreakPointList[i].ExceptionAddress, "\xCC", 1, &Bytes);

					//// 4. ��ԭ�ڴ�ı�������
					//VirtualProtectEx(ProcessHandle, BreakPointList[i].ExceptionAddress, 1, OldProtect, &OldProtect);

					//// 5. ���������ӵ�int3�ϵ㵽�б�
					//BreakPointList.push_back(Int3Info);

					////����������öϵ�
					//if (BreakPointList[i].EternalOrNot == 0)
					//{
					//	//��ԭ�����ڴ�ϵ�ɾ��
					//	BreakPointList.erase(BreakPointList.begin() + i);
					//}									
					//��������ڴ��쳣�Ķϵ�պ�ʱ�ڴ�ϵ��¶ϵ�λ�ã�����Ҫ�����û����룬�����ֱ��������
						return TRUE;		
				}
			}
		}
	}
	//������ǵ����������������쳣���򲻴���
	return FALSE;
}





#include "pch.h"
#include <iostream>
#include <windows.h>
#include "Capstone.h"
#include "debug.h"
#include "BreakPoint.h"
#include <Psapi.h>

#include "AssamblyEngine/XEDParse.h" // �������
#pragma comment(lib,"AssamblyEngine/XEDParse.lib")

//�������Ե�dll
#define DLL_PATH "E:\\cԴ��\\debugger demo\\Debug\\��������Dll.dll" 

//���ڽ�char�ַ���תΪwchar_�ַ���
#define  CHAR_TO_WCHAR(lpChar, lpW_Char) MultiByteToWideChar(CP_ACP, NULL, lpChar, -1, lpW_Char, _countof(lpW_Char));
#define  WCHAR_TO_CHAR(lpW_Char, lpChar) WideCharToMultiByte(CP_ACP, NULL, lpW_Char, -1, lpChar, _countof(lpChar), NULL, FALSE);

using namespace std;

//////////////////////////////////////////////////////////
//�������Բ����ؽṹ��
//�ýṹ�����ڱ����dll�л�õ�ģ������
struct Info
{
	char name[20];
};

//�ýṹ�����ڱ���ɹ�����Ĳ����ģ�����ƺ�ģ����
struct PluginInfo
{
	char name[20];
	HMODULE Module;
};

//�ú���Ϊdll���ж�dll�汾��Ϣ�ĺ��������������øú����ж��ܷ���ظò��
typedef int(*pfunc)(Info& info);

//�ú���ָ�����ڱ����dll����л��showfunc�ĺ������
typedef void(*pfunc2)();

//����������뵼����ĺ���ָ��
typedef void(*pfunc3)(DWORD dwPid);

//void GetImExTable(DWORD dwPid)

//��Ϊ������ܲ�����ֻ��һ�������Բ������Ϣ���ʹ�ö�̬������б���
vector<PluginInfo> plugins;

//�ú������ڼ��ز���Լ����в���к���
//���������еĲ�����ڽ��������Խ����и���ģ��ĵ��뵼����
void Debugger::GetModule()
{
	//���dll�ı���·����֮����������·����Ѱ�ҿɼ��ص�dll
	string path(".\\plugin\\");

	// ���ڴ���ļ���Ϣ 
	WIN32_FIND_DATAA FindData = { 0 };

	// ���Բ��Ҹ�·���º�׺Ϊ.dll���ļ�
	HANDLE FindHandle = FindFirstFileA(".\\plugin\\*.dll", &FindData);

	// ����ҵ���
	if (FindHandle != INVALID_HANDLE_VALUE)
	{
		// ƴ�ӳ�·��
		string currentfile = path + FindData.cFileName;

		// ���ز��DLL������
		HMODULE Handle = LoadLibraryA(currentfile.c_str());

		// �Ƿ���سɹ�
		if (Handle != NULL)
		{
			// ��ȡ�����жϲ���Ƿ���ϰ汾Ҫ��
			pfunc func = (pfunc)GetProcAddress(Handle, "getinfo");//���dll�з��ز���汾��Ϣ�ĺ���Ϊgetinfo

			// ���ò���ĺ����жϰ汾
			if (func != NULL)
			{
				Info info = { 0 };
				if (func(info) == 1)//�����ǹؼ������dll�еĺ�������ֵΪ1��˵����ǰ��������ڰ汾1��������˵���ò���������ڵ�ǰ������
				{
					// ���������������������Ϣ
					PluginInfo Plugin = { 0 };//��������Ϣ�ṹ��
					strcpy_s(Plugin.name, 20, info.name);//����������
					Plugin.Module = Handle;//�������ڽ����еľ��
					plugins.push_back(Plugin);//����ģ������ƺ;�������ڶ�̬���飬�����Ժ�Ӹ�ģ���е���������ʹ��GetProcAddress��
				
					//�Ӳ���л�� showfunc ����
					pfunc2 func2 = (pfunc2)GetProcAddress(Plugin.Module, "showfunc");
					
					func2();
						
					//����������뵼����ĺ���ָ��
					pfunc3 GetModuleIMEMinfo = (pfunc3)GetProcAddress(Plugin.Module, "GetImExTable");
			
					//ʹ�õ�ǰ�����Խ��̵ľ������ý���id�����ò���еĺ���
					GetModuleIMEMinfo(GetProcessId(ProcessHandle));
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////


typedef struct _EFLAGS
{
	unsigned CF : 1;
	unsigned Reserve1 : 1;
	unsigned PF : 1;
	unsigned Reserve2 : 1;
	unsigned AF : 1;
	unsigned Reserve3 : 1;
	unsigned ZF : 1;
	unsigned SF : 1;
	unsigned TF : 1;
	unsigned IF : 1;
	unsigned DF : 1;
	unsigned OF : 1;
	unsigned IOPL : 2;
	unsigned NT : 1;
	unsigned Reserve4 : 1;
	unsigned RF : 1;
	unsigned VM : 1;
	unsigned AC : 1;
	unsigned VIF : 1;
	unsigned VIP : 1;
	unsigned ID : 1;
	unsigned Reserve5 : 10;
}REG_EFLAGS,*PREG_EFLAGS;


//��ȡ32λ��64λģ�飬������Ӧ������mָ��
void Debugger::GetModule32and64(DWORD dwPid)
{
	//���ָ�����̵ľ��
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
	//ȷ���ض�������Ҫ�����ڴ洢����Ϣ
	DWORD dwBufferSize = 0;
	::EnumProcessModulesEx(hProcess, NULL, 0, &dwBufferSize, LIST_MODULES_ALL);

	//����ռ�洢ģ��������
	HMODULE* pModuleHandleArr = (HMODULE*)new char[dwBufferSize];

	//����ض���������ģ��ľ��
	::EnumProcessModulesEx(hProcess, pModuleHandleArr, dwBufferSize, &dwBufferSize, LIST_MODULES_ALL);

	for (int i = 0; i < dwBufferSize / sizeof(HMODULE); i++)
	{
		//�����������ģ����
		TCHAR szModuleName[MAX_PATH] = { 0 };

		//����ṹ�����ģ����Ϣ
		MODULEINFO stcModuleInfo = { 0 };

		//��ȡ���ģ�����Ϣ
		GetModuleInformation(hProcess, pModuleHandleArr[i], &stcModuleInfo, sizeof(MODULEINFO));

		//��ȡģ���ļ���������·��
		GetModuleFileNameEx(hProcess, pModuleHandleArr[i], szModuleName, MAX_PATH);

		//��ģ����ת��asicc��������
		char ModulePathASCII[MAX_PATH] = { 0 };
		WCHAR_TO_CHAR(szModuleName, ModulePathASCII);

		//���ģ����
		printf("ģ������%s ", ModulePathASCII);
		//��ģ���ַתΪ�ַ���
		cout<<"ģ���ַ��"<<stcModuleInfo.lpBaseOfDll<<"  ";
		//��ģ���ڴ��СתΪ�ַ���
		cout<<"ģ���С"<<stcModuleInfo.SizeOfImage<<"\n";
	}
	delete[] pModuleHandleArr;
}


// �����Ե��Եķ�ʽ����һ�����̣������ط�������ģ��
bool Debugger::open(char const* FileName)
{
	// ��������ʱ��Ҫʹ�õĽṹ�壬���ڽ��մ������Ľ��еĽ��̡��̵߳ľ����id
	PROCESS_INFORMATION ps = { 0 };
	STARTUPINFOA si = { sizeof(STARTUPINFO) };

	// ���ݴ�����ļ�[�Ե��Է�ʽ]����һ������
	BOOL isSuccess = CreateProcessA(FileName, NULL,
		NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS
		| CREATE_NEW_CONSOLE, NULL, NULL, &si, &ps);

	// �ж��Ƿ񴴽��ɹ���ʧ�ܷ���FALSE
	if (isSuccess == FALSE)
		return FALSE;

	////////////////////ע�뷴������ģ��////////////////////////////////
		//1.��ȡĿ����̾��
	HANDLE hProcess = OpenProcess(
		PROCESS_ALL_ACCESS,//��һ�����̣��ý��̵�Ȩ��Ϊ���п���ʹ�õ�Ȩ��
		FALSE, ps.dwProcessId);//�Ƿ���Լ̳У���Ҫ�򿪵Ľ��̵�pid
	if (!hProcess)
	{
		printf("���̴�ʧ��\n");
	}	
	//2.��Ŀ�����������һ���ڴ棨��С��DLL·���ĳ��ȣ�
	LPVOID lpBuf = VirtualAllocEx(hProcess,
		NULL,//���������ڴ���׵�ַ�����ΪNULL����ϵͳָ���׵�ַ 
		1, //�����ǰ����ȣ�4096�ֽڣ������ڴ棬д1Ҳ��һ����
		MEM_COMMIT,//��������ڴ������ύ����������ʹ��
		PAGE_READWRITE);//�ڴ汣��Ȩ��Ϊ�ɶ���д

	//3.��dll·��д�뵽Ŀ�������
	DWORD dwWrite;
	WriteProcessMemory(hProcess, //Ŀ����̾��
		lpBuf, DLL_PATH, //Ҫд���ڴ�ռ���׵�ַ��Ҫд�������
		MAX_PATH, &dwWrite);//Ҫд�����ݵĴ�С��ʵ��д����ڴ�Ĵ�С

	//4.����Զ���߳�
	//����Զ���̵߳Ļص�����ΪLoadLibrary�����ڽ�dll���뵽Ŀ�Ľ���
	//��Ϊ�ú���λ��kernelģ���У������ڲ�ͬ�Ľ������������ڴ��ַ����ͬ��
	//����ֱ�Ӵ���Ŀ�Ľ��̣�����Ŀ�Ľ��̵��øú���
	HANDLE hThread = CreateRemoteThread(hProcess,//Ŀ�����
		NULL, NULL, //��ȫ���������߳�ջ�Ĵ�С�����߾�ʹ��Ĭ��ֵ
		(LPTHREAD_START_ROUTINE)LoadLibraryA,//�½��̵߳Ļص�������ַ��ע�⺯��ָ������ͱ������ǿת
		lpBuf, 0, 0);//�ص����������������̵߳�״̬������ִ�У��������̵߳�id

	////////////////////////////////////////////////////ע�뷴������


	// DEBUG_ONLY_THIS_PROCESS �� DEBUG_PROCESS �������
	// ���Ƿ���Ա����Գ��򴴽����ӽ���

	// �رս��̺��̵߳ľ��
	CloseHandle(ps.hThread);
	CloseHandle(ps.hProcess);

	// �ڴ������ԻỰʱִ�з��ر�������ĳ�ʼ��
	Capstone::Init();

	return true;
}

//�����Ը��ӷ�ʽ�������Խ���
bool Debugger::attachment(DWORD dwPid) 
{

	//���з��������ĳ�ʼ��
	Capstone::Init();
	
	//GetModule(dwPid);

	return DebugActiveProcess(dwPid);
}


//�ú�������ʱ�̼�ر����Գ���������Ĵ����ϵ㣬�ú����������߳��з�ѭ������
VOID Debugger::ContionBpFind(){

	//�����ȡ���̻߳���
	CONTEXT RegInfo{ CONTEXT_ALL }; 
	
	GetThreadContext(ThreadHandle, &RegInfo);
	
	//����ǰ�����ϵ��������ĸ��Ĵ�����
	switch (ContionBpRegNumber)
	{
	case 1://eax��Ϊ�����Ĵ���
	{
		//��eax��ֵΪĳ��Ԥ���ֵʱ�����ָ��λ����int3�ϵ�
		//eaxΪĳ��Ԥ��ֵʱ
		if (RegInfo.Eax == ContionBpNumber)
		{
			//���ڱ�����Զ�̽��̶�д���ֽ���
			SIZE_T Bytes = 0;
			//�����ڸ���Զ���̱߳�������ʱ�����ڴ�ԭ�еı�������
			DWORD OldProtect = 0;

			//����һ���ϵ�ṹ�壬�ϵ�����Ϊcc�ϵ㣬�ϵ��ַΪAddressOfContionBp
			ExceptionInfo Int3Info = { CcFlag, (LPVOID)AddressOfContionBp };
			
			//�öϵ㲻�����öϵ�
			Int3Info.EternalOrNot = 0;

			// 1. �޸��ڴ�ı�������  �޸�һ���ڴ�ҳ��С���ڴ汣������
			VirtualProtectEx(ProcessHandle, (LPVOID)AddressOfContionBp, 1, PAGE_EXECUTE_READWRITE, &OldProtect);

			// 2. ��ȡ��ԭ�е����ݽ��б��棬��ȡһ�ֽڵ�Դ���뵽�ϵ�ṹ����
			ReadProcessMemory(ProcessHandle, (LPVOID)AddressOfContionBp, &Int3Info.u.OldOpcode, 1, &Bytes);

			// 3. �� 0xCC д��Ŀ��λ��
			WriteProcessMemory(ProcessHandle, (LPVOID)AddressOfContionBp, "\xCC", 1, &Bytes);

			// 4. ��ԭ�ڴ�ı�������
			VirtualProtectEx(ProcessHandle, (LPVOID)AddressOfContionBp, 1, OldProtect, &OldProtect);

			// 5. ����ϵ㵽�б�
			BreakPoint::BreakPointList.push_back(Int3Info);	
		}

		break;
	}	
	case 2://ebx��Ϊ�����Ĵ���
	{
		if (RegInfo.Ebx == ContionBpNumber)
		{
			//���ڱ�����Զ�̽��̶�д���ֽ���
			SIZE_T Bytes = 0;
			//�����ڸ���Զ���̱߳�������ʱ�����ڴ�ԭ�еı�������
			DWORD OldProtect = 0;

			ExceptionInfo Int3Info = { CcFlag, (LPVOID)AddressOfContionBp };

			Int3Info.EternalOrNot = 0;

			// 1. �޸��ڴ�ı�������  �޸�һ���ڴ�ҳ��С���ڴ汣������
			VirtualProtectEx(ProcessHandle, (LPVOID)AddressOfContionBp, 1, PAGE_EXECUTE_READWRITE, &OldProtect);

			// 2. ��ȡ��ԭ�е����ݽ��б��棬��ȡһ�ֽڵ�Դ���뵽�ϵ�ṹ����
			ReadProcessMemory(ProcessHandle, (LPVOID)AddressOfContionBp, &Int3Info.u.OldOpcode, 1, &Bytes);

			// 3. �� 0xCC д��Ŀ��λ��
			WriteProcessMemory(ProcessHandle, (LPVOID)AddressOfContionBp, "\xCC", 1, &Bytes);

			// 4. ��ԭ�ڴ�ı�������
			VirtualProtectEx(ProcessHandle, (LPVOID)AddressOfContionBp, 1, OldProtect, &OldProtect);

			// 5. ����ϵ㵽�б�
			BreakPoint::BreakPointList.push_back(Int3Info);
		}

		break;
	}
	case 3://ecx��Ϊ�����Ĵ���
	{
		if (RegInfo.Ecx == ContionBpNumber)
		{
			//���ڱ�����Զ�̽��̶�д���ֽ���
			SIZE_T Bytes = 0;
			//�����ڸ���Զ���̱߳�������ʱ�����ڴ�ԭ�еı�������
			DWORD OldProtect = 0;

			ExceptionInfo Int3Info = { CcFlag, (LPVOID)AddressOfContionBp };

			Int3Info.EternalOrNot = 0;

			// 1. �޸��ڴ�ı�������  �޸�һ���ڴ�ҳ��С���ڴ汣������
			VirtualProtectEx(ProcessHandle, (LPVOID)AddressOfContionBp, 1, PAGE_EXECUTE_READWRITE, &OldProtect);

			// 2. ��ȡ��ԭ�е����ݽ��б��棬��ȡһ�ֽڵ�Դ���뵽�ϵ�ṹ����
			ReadProcessMemory(ProcessHandle, (LPVOID)AddressOfContionBp, &Int3Info.u.OldOpcode, 1, &Bytes);

			// 3. �� 0xCC д��Ŀ��λ��
			WriteProcessMemory(ProcessHandle, (LPVOID)AddressOfContionBp, "\xCC", 1, &Bytes);

			// 4. ��ԭ�ڴ�ı�������
			VirtualProtectEx(ProcessHandle, (LPVOID)AddressOfContionBp, 1, OldProtect, &OldProtect);

			// 5. ����ϵ㵽�б�
			BreakPoint::BreakPointList.push_back(Int3Info);
		}

		break;
	}
	case 4://edx��Ϊ�����Ĵ���
	{
		if (RegInfo.Edx == ContionBpNumber)
		{
			//���ڱ�����Զ�̽��̶�д���ֽ���
			SIZE_T Bytes = 0;
			//�����ڸ���Զ���̱߳�������ʱ�����ڴ�ԭ�еı�������
			DWORD OldProtect = 0;

			ExceptionInfo Int3Info = { CcFlag, (LPVOID)AddressOfContionBp };

			Int3Info.EternalOrNot = 0;

			// 1. �޸��ڴ�ı�������  �޸�һ���ڴ�ҳ��С���ڴ汣������
			VirtualProtectEx(ProcessHandle, (LPVOID)AddressOfContionBp, 1, PAGE_EXECUTE_READWRITE, &OldProtect);

			// 2. ��ȡ��ԭ�е����ݽ��б��棬��ȡһ�ֽڵ�Դ���뵽�ϵ�ṹ����
			ReadProcessMemory(ProcessHandle, (LPVOID)AddressOfContionBp, &Int3Info.u.OldOpcode, 1, &Bytes);

			// 3. �� 0xCC д��Ŀ��λ��
			WriteProcessMemory(ProcessHandle, (LPVOID)AddressOfContionBp, "\xCC", 1, &Bytes);

			// 4. ��ԭ�ڴ�ı�������
			VirtualProtectEx(ProcessHandle, (LPVOID)AddressOfContionBp, 1, OldProtect, &OldProtect);

			// 5. ����ϵ㵽�б�
			BreakPoint::BreakPointList.push_back(Int3Info);
		}
		break;
	}
	}
}


//���̵߳Ļص�����������������
//������¿�һ���̣߳�ר������ʱ�̼�ر����Խ����еļĴ�����ֵ����ʹ�øú�����Ϊ�̻߳ص�������
//�����¿�һ���߳�ʱ�̼�ؼĴ���ֵ�ķ������������⣬�������̻߳ص��������µ�����
DWORD CALLBACK ThreadProc(LPVOID pArg)
{
	while (true)
	{
		Debugger*p = (Debugger*)pArg;
		//���½�����ѭ�����øú���������ʱ�̼��ָ���Ĵ������Ƿ�Ϊָ��ֵ�����������ָ���ڴ��ַ���¶ϵ�
		p->ContionBpFind();
		
		//printf("���߳� : TID:%d \n", GetCurrentThreadId()/*��ȡ��ǰ�߳�id*/);
		Sleep(500);
	}
	return 0;
}



// �ȴ������¼�
void Debugger::run()
{
	// ���ڱ��������Ϣ�Ĵ�����
	//DBG_EXCEPTION_NOT_HANDLED��ʾ�쳣δ������
	DWORD Result = DBG_EXCEPTION_NOT_HANDLED;

	///////////////////////////////////////////////////////
	//���������½�һ����������ʱ�̼�ⱻ���Խ�����ָ���Ĵ�����ֵ��
	//������Ϊ�Ƿ��������µ����ݣ����Ǻ����������
		////�ñ������ڽ��մ����̵߳�id
		//DWORD tid = 0;
		//// ����һ���߳�
		//CreateThread(NULL,/*�ں˶���İ�ȫ������*/
		//	0,/*���߳�ջ�Ĵ�С, 0��ʾĬ�ϴ�С*/
		//	ThreadProc,/*���̵߳Ļص�����*/
		//	0,/*���Ӳ���*/
		//	0,/*������־*/
		//	&tid);/*�������̵߳�id*/

		//�����̵߳õ��߳̾��
		//HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME,/*�����Ȩ��*/
			//FALSE,/*�Ƿ�̳�*/
			//tid);/*Ҫ�򿪵��߳�id*/

	//////////////////////////////////////////////////////
	//��ѭ��������һֱ�ȴ����Զ��󷵻ص����¼�
	while (TRUE)
	{
		//�ú����ᵼ�µ��øú������̱߳�����ֱ����õ�����ϵͳ���ػ����ĵ����¼�
		//DebugEvent��debug��ĳ�Ա����
		WaitForDebugEvent(&DebugEvent, INFINITE);

		// �����쳣������λ�ô򿪾��
		//�쳣�Ľ��̺��߳̾����Ϊdebug��ĳ�Ա����
		OpenHandles();

		// ���ݵȴ����Ĳ�ͬ�����¼����д���
		switch (DebugEvent.dwDebugEventCode)
		{
			// ���յ��˲������쳣��Ϣ
		case EXCEPTION_DEBUG_EVENT:
			// �����쳣����ֻ�е���������ȷ�����˵������������Գ����µ��쳣ʱ��result��ΪDBG_CONTINUE
			//���౻���Գ����Լ��������쳣������������
			Result = OnExceptionHanlder();
			break;

			// ���յ��˽��̵Ĵ����¼�
		case CREATE_PROCESS_DEBUG_EVENT:
			//��ȡOEP������֮����OEP���¶ϵ�
			
			//�����ӽ��̷�ʽ���Ե�ʱ�򣬴ӱ����Գ��򴦻�õ��쳣�¼��У�
			//oep��ַʼ��Ϊ0������ȡ�ĵ����¼�DebugEvent�д���oep���ֶ�lpStartAddress��
			//�����޷�������oep���¶ϵ㣬ֻ����ϵͳ�ϵ㴦�Ͷ��²��ȴ�����
			StartAddress = DebugEvent.u.CreateProcessInfo.lpStartAddress;
//			StartAddress = DebugEvent.u.CreateThread.lpStartAddress;

			Result = DBG_CONTINUE;
			break;
		case LOAD_DLL_DEBUG_EVENT:
		{
			
			printf("��ģ����ء���ģ������%X����ģ����ػ�ַ��%X����ģ�����Ƶ�ַ��%X��\n", 
				DebugEvent.u.LoadDll.hFile, DebugEvent.u.LoadDll.lpBaseOfDll,DebugEvent.u.LoadDll.lpImageName);

			// �ڴ������̺�ģ����ص�ʱ�򣬵�����Ϣ�б����� 
			// lpImageName �� fUnicode����������Ϣͨ��
			// ��û���õģ���Ӧ��ʹ����

			////���쳣�¼��л�õ�ģ�������ַ����ĵ�ַ�����⵼�£�����ַ�����ʱ�����ڴ���ʴ���
			//if (DebugEvent.u.LoadDll.lpImageName == NULL)
			//{
			//	cout << "����ģ��������NULL��\n";
			//}
			//else
			//{
			//	/*TCHAR*pName = NULL;
			//	pName = (TCHAR*)DebugEvent.u.LoadDll.lpImageName;
			//	cout << "����ģ��������" << pName << "��\n";*/

			//	char*pName = NULL;
			//	pName = (char*)DebugEvent.u.LoadDll.lpImageName;

			//	int  NumberOfModuleName = atoi(pName);
			//	char* AdderssOfModuleName = (char*)NumberOfModuleName;
			//	cout << "����ģ��������" << AdderssOfModuleName << "��\n";
			//}
			Result = DBG_CONTINUE;
		}
			break; 
			// �������Ҳ����δ����
		default:
			Result = DBG_EXCEPTION_NOT_HANDLED;
			break;
		}

		// �ظ�������ϵͳ��ǰ������Ϣ�Ƿ񱻴���
		// �����е� PID �� TID �����ǵȴ�����
		// �¼�ʱ�������� ID��
		
		//�����ϵ��⣬�������ָ���Ĵ����Ƿ�Ϊ�ض�ֵ�����������ض���ַ�¶ϵ�
		//�������������ϵ�������ڳ�����µ�ʱ����ܽ��мĴ���ֵ�ü��
		ContionBpFind();
		
		//ע��������������Գ������������쳣����������ڴ���ʴ���ȣ�
		//�����쳣��Ҫ�����Գ����Լ����������Ҫ�����쳣�����¼��ĺ����ķ���ֵ�����쳣���ͷ��ز�ͬ��ֵ
		ContinueDebugEvent(
			DebugEvent.dwProcessId,
			DebugEvent.dwThreadId,
			// ��������ʾ�Ƿ���������¼�����
			// �������˷��� DBG_CONTINUE
			Result);

		//�������öϵ㣬��֤��һ�λ��ܱ���ס
		BreakPoint::ReSetCcBreakPointOfEnternal(ProcessHandle,ThreadHandle);

		// �رվ��
		CloseHandles();
	}
}

// ר�����ڴ����쳣��Ϣ
DWORD Debugger::OnExceptionHanlder()
{
	//Ϊ�˱�֤�����Խ����Լ������Լ��������쳣����Ĭ���쳣û�б�����������
	//�����⵽�쳣�ǵ������������Գ����µģ��Ž����쳣�����������쳣����ɹ�
	DWORD Result = DBG_EXCEPTION_NOT_HANDLED;

	// �ӵ����¼��л�ȡ  �쳣����ʱ�ĵ�ַ  ��  �쳣������
	DWORD ExceptionCode = DebugEvent.u.Exception.ExceptionRecord.ExceptionCode;
	LPVOID ExceptionAddress = DebugEvent.u.Exception.ExceptionRecord.ExceptionAddress;


	//�������һ��Ԫ�ر�ʾ�ڴ�����쳣�����쳣��ʽ 0����ȡʱ�쳣   1��д��ʱ�쳣   8��ִ��ʱ�쳣
	//������ڶ���Ԫ�ر�ʾ�����쳣�����������ַ����������д�쳣ʱ��
	//ULONG_PTR ���������������Ϊunsigned long

	//������ĵڶ���Ԫ�ر������ڴ����д��ִ��ʱ����쳣�ĵ�ַ������ڴ��쳣��ֱ��ʹ�øõ�ַ��ַ�Ϳ���
	ULONG_PTR* MemBpExceptionInfo = DebugEvent.u.Exception.ExceptionRecord.ExceptionInformation;
  
	//���ڱ�ʶ����ʾ�ڴ�ϵ��ַ����������ַ�������ڽ����û������ʱ������ж�
	BOOL IsMemBpOrNot = FALSE;

	// ���ݲ�ͬ���쳣����ִ�в�ͬ�Ĳ���
	switch (ExceptionCode)
	{
	// �ڴ�ϵ���޸�
	case EXCEPTION_ACCESS_VIOLATION://�ú��������ָ���д��ִ���˲�����ӦȨ�޵��ڴ�
	{
		// �����������ڴ��ҳ����Ϊ ֻ��������ִ�С����ɶ�д
		// ��ҳ���Ե������� ��ҳ��СΪ��λ
		
		//���������Ƿ���ȷ�޸����ڴ�ϵ�
		//�����Կ��ǣ�Ĭ��û���޸�
		BOOL FixResult = FALSE;

		//�ò��������жϵ�ǰ�����쳣�ĵ�ַ�Ƿ�պ������ڴ�ϵ�ĵ�ַ,����ǣ�����Ҫ�����û�����
		int IsOrNot = 0;

		//�޸��ڴ�ϵ㣬����Ҫ�����ڴ�ϵ�����ͣ�ֱ�ӽ��ϵ㴦���ڴ汣������ֱ�ӻ�ԭ
		//�������������Ҫ�޸������쳣���������ڴ�ϵ�ͬһ�ڴ�ҳ�������ڴ��ַ��
		//��Ҫ�޸��ڴ�ҳ�ķ������ԣ��Ӷ�ʹ������Լ���ִ�У�Ȼ���ٴν����ڴ�ϵ���¶�
		FixResult=BreakPoint::FixMemBreakPoint(ProcessHandle, ThreadHandle,(LPVOID)MemBpExceptionInfo[1],&IsOrNot);//�����Ƿ����쳣�����������ַ �� �����ڴ�����쳣������

		//ע������Ҳ��Ҫ���ǵ�ǰ�Ƿ��쳣�ɹ���������ǵ������������쳣�����Ѿ��������ܷ���DBG_CONTINUE�������ܷ����쳣�Ѵ���
		if (FixResult)//����쳣����ɹ����ܷ���DBG_CONTINUE;		
		{
			Result = DBG_CONTINUE;
		}
		if (!IsOrNot)//��������쳣�ĵ�ַ���������ڴ�ϵ�ĵ�ַ������Ҫ�����û����룬������Ҫ�����û�����
		{
			NeddCommand = FALSE;
		}
		//����������ڽ����û������ʱ��
		IsMemBpOrNot = TRUE;
		break;
	}
	// ����ϵ���޸�
	case EXCEPTION_BREAKPOINT:
	{
		// 1. �ж��ǲ���ϵͳ�ϵ㣬��ϵͳ�ڵ��Գ�������е�ʱ���Զ����õĶϵ�
		//�������Խ��̵ĵ�һ������ϵ���ϵͳ���õģ��Ƿ����иöϵ���isSystemBreakPoint������ʶ
		if (isSystemBreakPoint == TRUE)
		{
			// 2. �� OEP ��λ������һ������ϵ�
			//oep�Ļ�ȡ���ڱ����Խ��̴�����ʱ���ȡ����
			//����oep��������ϵ�ʱ����Ҫ�����Ƿ񽫸öϵ�����Ϊ���öϵ��ѡ��
			BreakPoint::SetCcBreakPoint(ProcessHandle, StartAddress);

			// 3. ��һ�ξͲ���ϵͳ�ϵ���
			isSystemBreakPoint = FALSE;

			// 4.�������ѡ��������Ƿ��ڳ���ϵͳ�ϵ��ж���
			//�����Ƿ���ϵͳ�ϵ㴦���£������Խ��̴���֮�󶼻���oep���¶ϵ�
			//���ڸ��ӽ�����˵����Ϊ�������Ľ����Ѿ����������ˣ�����ϵͳ�ϵ��ʹΪoep��Ӷϵ�Ҳ�����ס����ʱ������Ҫ�û���������
			
			NeddCommand = TRUE;//��ϵͳ�ϵ㴦����
			//NeddCommand = FALSE;//����ϵͳ�ϵ㴦���£�ֱ����oep������

			//ϵͳ�ϵ㴦��ɹ�
			Result = DBG_CONTINUE;
			break;
		}
		// �޸���ǰ�Լ����õ�����ϵ�
		
		//���int3�ϵ㴦��ɹ����򷵻ش���ɹ����
		//int3�ϵ㴦��ɹ�
		if (BreakPoint::FixCcBreakPoint(ProcessHandle, ThreadHandle, ExceptionAddress))
		{
			//���FixCcBreakPoint����ִ�гɹ���˵����ʱ�����Լ����õ�cc�ϵ㲢�ҳɹ��޸�������result����dbg_continue
			//������еĲ����Լ����õĶϵ㣬��Ӧ����FixCcBreakPoint����0���Ӷ���֤result��ֵΪDBG_EXCEPTION_NOT_HANDLED
			//�����쳣��Ӧ���õ���������
			Result = DBG_CONTINUE;
		}
		break;
	}

	// Ӳ���ϵ���޸�
	case EXCEPTION_SINGLE_STEP:
	{
		//�Ƿ���Ҫ�û�����
		//Ӳ���ϵ�ָ�֮����ʱ��Ҫ�ȴ��û����룬��ʱ�����û����룬
		//�ñ������ڴ�fixhdbreakpoint�����з����Ƿ���Ҫ�����û��������Ϣ
		int IsOrNot = 0;

		// �޸�Ӳ���ϵ㣬�ó������ִ��
		if(BreakPoint::FixHdBreakPoint(ProcessHandle,ThreadHandle, ExceptionAddress,&IsOrNot))
		{
			Result = DBG_CONTINUE;
		}
		if (IsOrNot == 1)//��Ҫ�����û�����
		{
			NeddCommand == TRUE;
		}
		if (IsOrNot == 0)//����Ҫ�����û�����
		{
			NeddCommand == FALSE;
		}
		break;
	}
	}

	//����ϵͳ��ԭ���������ģ������Խ������Ա����Եķ�ʽ�򿪺�
	//���Զ�����ϵͳ�쳣��λ�ã����µľ���ԭ���Ǽ����ض�λ���׳�ϵͳ�쳣��������쳣��
	//��ʱ�쳣�¼��ȴ������ᱻ���ã������ϵͳ�쳣������oep�����öϵ㣬
	//���ص�����ϵͳ���쳣����ɹ����������б����Գ����ָ��
	//�����Գ������е�oep��ʱ��ͬ�������쳣�¼���Ϣ��ͨ��������ϵͳ���ݸ���������
	//��������ִ�жϵ���޸���֮������ס�ȴ��û�����������ͬʱ��ӡ������
	//�û����������ɶϵ�����ã�Ϊ������һ����ס����׼����
	//֮��������ظ�������ϵͳ���쳣�Ѿ������������Գ���������У�ֱ����һ�α�����
	
	// �����Ҫ���²���������
	DWORD OldProtect;//�����ڴ�ҳԭ�����ڴ汣������
	if (NeddCommand == TRUE)//�����Ҫ���²���������
	{	
		if (IsMemBpOrNot)//�����ǰ�ϵ�Ϊ�ڴ�ϵ�
		{
			//������Ϊ������ϵ㴦��������룬������Ҫ���ڴ����Խ��и��ģ��Ӷ���������ڴ��ȡ
			VirtualProtectEx(ProcessHandle, (LPVOID)MemBpExceptionInfo[1], 1, PAGE_READWRITE, &OldProtect);
			Capstone::DisAsm(ProcessHandle, (LPVOID)MemBpExceptionInfo[1], 10);
			VirtualProtectEx(ProcessHandle, (LPVOID)MemBpExceptionInfo[1], 1, OldProtect, &OldProtect);
			IsMemBpOrNot = FALSE;
		}
		else {
			Capstone::DisAsm(ProcessHandle, ExceptionAddress, 10);	
		}
		GetCommand();
	}

	// �����Ƿ���Ҫ����
	NeddCommand = TRUE;

	return Result;
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

//��ʾ�Ĵ�����Ϣ  ע��eflag��־λ�Լ��μĴ�����ʹ�ı��ˣ�����һ�λ�ȡ��ʱ���ǻḴԭ
VOID Debugger::ShowRegister(HANDLE ThreadHandle)
{	

	//�Ƿ���Ҫ�޸ļĴ����ı�־
	int ChangeRegOrNot = 0;
	//����eflags�Ĵ����Ľṹ��
	PREG_EFLAGS pEflags = NULL;
	//�����ȡ���̻߳���
	CONTEXT RegInfo{CONTEXT_ALL};
	//������Ҫ�޸��ļĴ����ı�־
	int ChangeIdOfReg = 0;
	//���ڱ���CONTEXT�ṹ����ÿһ���Ĵ���ֵ�ĵ�ַ
	PDWORD ArrOfReg[15];
	//���ڽṹ�޸ĺ�ļĴ�����ֵ
	DWORD ChangeNumber = 0;

	GetThreadContext(ThreadHandle, &RegInfo);
	
	ArrOfReg[0] = &RegInfo.Eax;
	ArrOfReg[1] = &RegInfo.Ecx;
	ArrOfReg[2] = &RegInfo.Edx;
	ArrOfReg[3] = &RegInfo.Ebx;
	ArrOfReg[4] = &RegInfo.Esi;
	ArrOfReg[5] = &RegInfo.Edi;
	ArrOfReg[6] = &RegInfo.Esp;
	ArrOfReg[7] = &RegInfo.Ebp;
	ArrOfReg[8] = &RegInfo.SegCs;
	ArrOfReg[9] = &RegInfo.SegSs;
	ArrOfReg[10] = &RegInfo.SegDs;
	ArrOfReg[11] = &RegInfo.SegEs;
	ArrOfReg[12] = &RegInfo.SegFs;
	ArrOfReg[13] = &RegInfo.SegGs;
	ArrOfReg[14] = &RegInfo.Eip;

	printf("\tEAX:%5X  ECX:%5X  EDX:%5X  EBX:%5X\n\t", RegInfo.Eax, RegInfo.Ecx, RegInfo.Edx, RegInfo.Ebx);

	printf("ESI:%5X  EDI:%5X  ESP:%5X  EBP:%5X\n\t", RegInfo.Esi, RegInfo.Edi, RegInfo.Esp, RegInfo.Ebp);
	
	printf("SegCs:%5X  SegSs:%5X  SegDs:%5X  SegEs:%5X  SegFs:%5X  SegGs:%5X\n\t",
		    RegInfo.SegCs, RegInfo.SegSs, RegInfo.SegDs, RegInfo.SegEs, RegInfo.SegFs, RegInfo.SegGs);

	printf("EIP:%5X\n\t", RegInfo.Eip);

	//�ܿ���ʱ��Ӧ�ڴ�ռ�û����Ӧ��дȨ��
	//memcpy((void*)&pEflags, (void*)RegInfo.ContextFlags, sizeof(DWORD));
	pEflags = (PREG_EFLAGS)&RegInfo.ContextFlags;

	printf("CF:%5X  ZF:%5X  SF:%5X\n\t", pEflags->CF, pEflags->ZF, pEflags->SF);
	printf("PF:%5X  OF:%5X  AF:%5X\n\t", pEflags->PF, pEflags->OF, pEflags->AF);
	printf("DF:%5X  IF:%5X  TF:%5X\n", pEflags->DF, pEflags->IF, pEflags->TF);

	cout << "�Ƿ��޸ļĴ�����1���޸�/0�����޸ģ���";

	cin >> ChangeRegOrNot;
	if (ChangeRegOrNot == 1)//�޸ļĴ���
	{
		cout << "������Ҫ�޸ĵļĴ������\n";
		cout << "EAX:0  ECX:1  EDX:2  EBX:3\n";
		cout << "ESI:4  EDI:5  ESP:6  EBP:7\n";
		cout << "SegCs:8  SegSs:9  SegDs:10  SegEs:11  SegFs:12  SegGs:13\n";
		cout << "EIP:14\n";
		cout << "CF:15  ZF:16  SF:17\n";
		cout << "PF:18  OF:19  AF:20\n";
		cout << "DF:21  IF:22  TF:23\n";
	
		cin >> ChangeIdOfReg;

		if (ChangeIdOfReg >= 0 && ChangeIdOfReg <= 14)
		{
			cout << "�����޸ĺ��ֵ\n";
			scanf_s("%x", &ChangeNumber);
			
			//cin >> ChangeNumber;
			
			*ArrOfReg[ChangeIdOfReg] = ChangeNumber;
		}
		else if (ChangeIdOfReg >= 15 && ChangeIdOfReg <= 23)
		{
			/*	printf("CF:%5X  ZF:%5X  SF:%5X\n\t", pEflags->CF, pEflags->ZF, pEflags->SF);
				printf("PF:%5X  OF:%5X  AF:%5X\n\t", pEflags->PF, pEflags->OF, pEflags->AF);
				printf("DF:%5X  IF:%5X  TF:%5X\n", pEflags->DF, pEflags->IF, pEflags->TF);*/

			cout << "�����޸ĺ��ֵ(0��1)\n";
			scanf_s("%x", &ChangeNumber);
			
			//cin >> ChangeNumber;

			if (ChangeNumber == 1 || ChangeNumber == 0)
			{
				switch (ChangeIdOfReg)
				{
				case 15:
					pEflags->CF = ChangeNumber;
					break;
				case 16:
					pEflags->ZF = ChangeNumber;
					break;
				case 17:
					pEflags->SF = ChangeNumber;
					break;
				case 18:
					pEflags->PF = ChangeNumber;
					break;
				case 19:
					pEflags->OF = ChangeNumber;
					break;
				case 20:
					pEflags->AF = ChangeNumber;
					break;
				case 21:
					pEflags->DF = ChangeNumber;
					break;
				case 22:
					pEflags->IF = ChangeNumber;
					break;
				case 23:
					pEflags->TF = ChangeNumber;
					break;
				}
			}
		}
		else
			cout << "������Ŵ���";
		// 3. ���޸ĵļĴ����������õ�Ŀ���߳�
		SetThreadContext(ThreadHandle, &RegInfo);
	}
	cout << "����ָ����������룺t�� ������ϵ㣺bp�� ��Ӳ���ϵ㣺hbp�� ���鿴/�޸ļĴ�����reg�� ���鿴/�޸Ļ�ࣺasm��" << "\n"
		<< "          ���ڴ������men�� ��ջ������sta�� ���ڴ�ϵ㣺mbp�� ������������p�� �������ϵ㣺c�� ��ģ����Ϣ��m��" << "\n"
		<< "          �����ز����plugin��";


}


VOID Debugger::ChangeASM(HANDLE ProcessHandle) 
{	
	//ʵ��ͨ��OPCODE�޸Ļ������
	////���ڱ�����Զ�̽��̶�д���ֽ���
	//SIZE_T Bytes = 0;
	////�����ڸ���Զ���̱߳�������ʱ�����ڴ�ԭ�еı�������
	//DWORD OldProtect = 0;	
	//
	//DWORD Address = 0;
	//cout << "����Ҫ�޸ĵĵ�ַ: ";
	//scanf_s("%x", &Address);
	//
	//int ChangeByte = 0;
	//cout << "����Ҫ�޸ĵ��ֽ���: ";
	//scanf_s("%d", &ChangeByte);
	//
	//char*ChangeDataChar = new char[ChangeByte+2];
	//DWORD ChangeData = 0;
	//cout << "����Ҫ�޸ĵ�����(ʮ��������): ";
	//cin >> ChangeDataChar;
	//ChangeData = strtol(ChangeDataChar, NULL, 16);


	//if (sizeof(ChangeData)-3 != ChangeByte)
	//{
	//	cout << "�������ݵĴ�С������Ҫ�������޸Ĳ���ʧ��";
	//	return;
	//}

	//// 1. �޸��ڴ�ı�������  �޸�ָ�����ֽڵ��ڴ汣������
	//VirtualProtectEx(ProcessHandle, (LPVOID)Address, ChangeByte, PAGE_EXECUTE_READWRITE, &OldProtect);

	//WriteProcessMemory(ProcessHandle, (LPVOID)Address, (LPVOID)&ChangeData, ChangeByte, &Bytes);
	//// 3. ������д��Ŀ��λ��angeData, ChangeByte, &Bytes);
	//if (Bytes != ChangeByte)
	//{
	//	cout << "����д��ʧ��";
	//}
	//// 4. ��ԭ�ڴ�ı�������
	//VirtualProtectEx(ProcessHandle, (LPVOID)Address, ChangeByte, OldProtect, &OldProtect);

	//delete[]ChangeDataChar;


	//���ڱ�����Զ�̽��̶�д���ֽ���
	SIZE_T Bytes = 0;
	//�����ڸ���Զ���̱߳�������ʱ�����ڴ�ԭ�еı�������
	DWORD OldProtect = 0;	


	XEDPARSE xed = { 0 };
	printf("��ַ��");

	// ��������opcode�ĵĳ�ʼ��ַ
	scanf_s("%x", &xed.cip);
	getchar();

	do
	{
		// ����ָ��
		printf("ָ�");
		gets_s(xed.instr, XEDPARSE_MAXBUFSIZE);

		// xed.cip, ��������תƫ�Ƶ�ָ��ʱ,��Ҫ��������ֶ�
		if (XEDPARSE_OK != XEDParseAssemble(&xed))
		{
			printf("ָ�����%s\n", xed.error);
			continue;
		}

		// ��ӡ���ָ�������ɵ�opcode
		printf("%08X : ", xed.cip);

		for (int i = 0; i < xed.dest_size; ++i)
		{
			printf("%02X ", xed.dest[i]);
		}		
		printf("\n");

		// 1. �޸��ڴ�ı�������  �޸�ָ�����ֽڵ��ڴ汣������
		VirtualProtectEx(ProcessHandle, (LPVOID)xed.cip, xed.dest_size, PAGE_EXECUTE_READWRITE, &OldProtect);

		WriteProcessMemory(ProcessHandle, (LPVOID)xed.cip, (LPVOID)xed.dest, xed.dest_size, &Bytes);
		// 3. ������д��Ŀ��λ��angeData, ChangeByte, &Bytes);
		if (Bytes != xed.dest_size)
		{
			cout << "����д��ʧ��\n";
		}
		// 4. ��ԭ�ڴ�ı�������
		VirtualProtectEx(ProcessHandle, (LPVOID)xed.cip, xed.dest_size, OldProtect, &OldProtect);

		// ����ַ���ӵ���һ��ָ����׵�ַ
		xed.cip += xed.dest_size;

		BOOL ContinueChangeOrNot=0;
		cout << "�Ƿ�����޸Ļ�ࣿ��1���� / 0����";
		cin >> ContinueChangeOrNot;
		//������Ҫ��ջ���������֤�س���������Ϊ�����޸ĵ�ָ���ȡ
		
		//������뻺��������ֹ��������µ�bug
		std::cin.clear();  //����cin��־λ
		std::cin.ignore(1024, '\n');



		if (!ContinueChangeOrNot)
		{
			cout << "����ָ����������룺t�� ������ϵ㣺bp�� ��Ӳ���ϵ㣺hbp�� ���鿴/�޸ļĴ�����reg�� ���鿴/�޸Ļ�ࣺasm��" << "\n"
				<< "          ���ڴ������men�� ��ջ������sta�� ���ڴ�ϵ㣺mbp�� ������������p�� �������ϵ㣺c�� ��ģ����Ϣ��m��" << "\n"
				<< "          �����ز����plugin��";
			break;
		}
	} while (*xed.instr);
}


VOID Debugger::ChangeMEM(HANDLE ProcessHandle)
{

	//���ڱ�����Զ�̽��̶�д���ֽ���
	SIZE_T Bytes = 0;
	//�����ڸ���Զ���̱߳�������ʱ�����ڴ�ԭ�еı�������
	DWORD OldProtect = 0;
	//���ڱ�����Ҫ��ѯ���ڴ��ַ
	DWORD Address = 0;
	//������ڴ��ж���������,Ĭ��һ����ʾ64���ֽڵ�����
	unsigned char MemData[64] = { 0 };
	//�����޸��������
	BOOL ChangeOrNot = FALSE;
	//���ڱ�����Ҫ�޸ĵ��ڴ��ַ
	DWORD AddressOfChange = 0;

	//�Ƿ�����޸��ڴ�
	BOOL ContinueOrNot = FALSE;

	cout << "������Ҫ��ѯ���ڴ��ַ";
	scanf_s("%x", &Address);

	// 1. �޸��ڴ�ı�������  �޸�һ���ֽڵ��ڴ汣������
	VirtualProtectEx(ProcessHandle, (LPVOID)Address, 64, PAGE_EXECUTE_READWRITE, &OldProtect);

	// 2. ��ȡ��ԭ�е����ݽ��б��棬��ȡһ�ֽڵ�Դ���뵽�ϵ�ṹ����
	ReadProcessMemory(ProcessHandle, (LPVOID)Address, &MemData, 64, &Bytes);

	if (Bytes != 64)
	{
		cout << "���ݶ�ȡʧ��";
	}

	//����ڴ����ݺͶ�Ӧ��ַ
	printf("%X   ", Address);
	for (int i = 0; i < 64; i++)
	{
		printf("%02X  ", MemData[i]);
		Address += 1;
		if ((i+1) % 16 == 0)
		{
			printf("\n");
			if (i != 63)
			{
				printf("%X   ", Address);
			}
		}
	}

	cout << "�Ƿ���Ҫ�޸��ڴ����ݣ���1���� / 0����";
	cin >> ChangeOrNot;
	if (ChangeOrNot)
	{
		cout << "������Ҫ�޸ĵ��ڴ��ַ";
		scanf_s("%x", &AddressOfChange);

		do {
			char ChangeDataChar[3] = { 0 };//һ���Ըı�һ���ֽ�����
			DWORD ChangeData = 0;
			cout << "����Ҫ�޸ĵ�����(��λʮ��������): ";
			cin >> ChangeDataChar;
			ChangeData = strtol(ChangeDataChar, NULL, 16);

			// 3. ��ָ������д��Ŀ��λ��
			WriteProcessMemory(ProcessHandle, (LPVOID)AddressOfChange, (LPVOID)&ChangeData, 1, &Bytes);

			if (Bytes != 1)
			{
				cout << "����д��ʧ��";
			}

			AddressOfChange += 1;

			cout << "�����޸�?��1���� / 0����: ";
			cin >> ContinueOrNot;
		} while (ContinueOrNot);
	}
	// 4. ��ԭ�ڴ�ı�������
	VirtualProtectEx(ProcessHandle, (LPVOID)Address, 1, OldProtect, &OldProtect);

	cout << "����ָ����������룺t�� ������ϵ㣺bp�� ��Ӳ���ϵ㣺hbp�� ���鿴/�޸ļĴ�����reg�� ���鿴/�޸Ļ�ࣺasm��" << "\n"
		<< "          ���ڴ������men�� ��ջ������sta�� ���ڴ�ϵ㣺mbp�� ������������p�� �������ϵ㣺c�� ��ģ����Ϣ��m��" << "\n"
		<< "          �����ز����plugin��";
}


VOID Debugger::ChangeStack(HANDLE ProcessHandle, HANDLE ThreadHandle)
{
	//���ڱ�����Զ�̽��̶�д���ֽ���
	SIZE_T Bytes = 0;
	//�����ڸ���Զ���̱߳�������ʱ�����ڴ�ԭ�еı�������
	DWORD OldProtect = 0;
	//���ڽ���ջ�ĵ�ַ
	DWORD Address = 0;

	//������ڴ��ж���������,Ĭ��һ����ʾ64���ֽڵ�����
	unsigned char MemData[64] = { 0 };
	//�����޸��������
	BOOL ChangeOrNot = FALSE;
	//���ڱ�����Ҫ�޸ĵ��ڴ��ַ
	DWORD AddressOfChange = 0;

	//�Ƿ�����޸��ڴ�
	BOOL ContinueOrNot = FALSE;

	//�����ȡ���̻߳���
	CONTEXT RegInfo{ CONTEXT_ALL };


	GetThreadContext(ThreadHandle, &RegInfo);
	Address = RegInfo.Esp;


	// 1. �޸��ڴ�ı�������  �޸�һ���ֽڵ��ڴ汣������
	VirtualProtectEx(ProcessHandle, (LPVOID)Address, 64, PAGE_EXECUTE_READWRITE, &OldProtect);

	// 2. ��ȡ��ԭ�е����ݽ��б��棬��ȡһ�ֽڵ�Դ���뵽�ϵ�ṹ����
	ReadProcessMemory(ProcessHandle, (LPVOID)Address, &MemData, 64, &Bytes);

	if (Bytes != 64)
	{
		cout << "���ݶ�ȡʧ��";
	}

	//����ڴ����ݺͶ�Ӧ��ַ
	printf("%X   ", Address);
	for (int i = 0; i < 64; i++)
	{
		printf("%02X", MemData[i]);
		Address += 1;
		//printf(" ");
		if ((i + 1) % 4 == 0)
		{
			printf("\n");

			if (i != 63)
			{
				printf("%X   ", Address);
			}
		}
	}
	cout << "�Ƿ���Ҫ�޸��ڴ����ݣ���1���� / 0����";
	cin >> ChangeOrNot;
	if (ChangeOrNot)
	{
		cout << "������Ҫ�޸ĵ��ڴ��ַ";
		scanf_s("%x", &AddressOfChange);

		do {
			char ChangeDataChar[3] = { 0 };//һ���Ըı�һ���ֽ�����
			DWORD ChangeData = 0;
			cout << "����Ҫ�޸ĵ�����(��λʮ��������): ";
			cin >> ChangeDataChar;
			ChangeData = strtol(ChangeDataChar, NULL, 16);

			// 3. ��ָ������д��Ŀ��λ��
			WriteProcessMemory(ProcessHandle, (LPVOID)AddressOfChange, (LPVOID)&ChangeData, 1, &Bytes);

			if (Bytes != 1)
			{
				cout << "����д��ʧ��";
			}

			AddressOfChange += 1;

			cout << "�����޸�?��1���� / 0����: ";
			cin >> ContinueOrNot;
		} while (ContinueOrNot);
	}
	// 4. ��ԭ�ڴ�ı�������
	VirtualProtectEx(ProcessHandle, (LPVOID)Address, 1, OldProtect, &OldProtect);

	cout << "����ָ����������룺t�� ������ϵ㣺bp�� ��Ӳ���ϵ㣺hbp�� ���鿴/�޸ļĴ�����reg�� ���鿴/�޸Ļ�ࣺasm��" << "\n"
		<< "          ���ڴ������men�� ��ջ������sta�� ���ڴ�ϵ㣺mbp�� ������������p�� �������ϵ㣺c�� ��ģ����Ϣ��m��" << "\n"
		<< "          �����ز����plugin��";
}


/*bool BreakPoint::SetCcBreakPoint(HANDLE ProcessHandle, LPVOID Address)
{
	// ����ϵ��ԭ������޸�Ŀ������еġ���һ���ֽڡ�Ϊ
	// 0xCC���޸���ʱ����Ϊ int 3 ��������һ����������
	// ��������ָ�������һ��ָ���λ�ã���ô��Ҫ�� eip ִ
	// �м�����������ԭָ��

	//���ڱ�����Զ�̽��̶�д���ֽ���
	SIZE_T Bytes = 0;
	//�����ڸ���Զ���̱߳�������ʱ�����ڴ�ԭ�еı�������
	DWORD OldProtect = 0;

	// 0. ����ϵ���Ϣ�Ľṹ�� �ϵ����� �ϵ��ַ
	ExceptionInfo Int3Info = { CcFlag, Address };

	// 1. �޸��ڴ�ı�������  �޸�һ���ֽڵ��ڴ汣������
	VirtualProtectEx(ProcessHandle, Address, 1, PAGE_EXECUTE_READWRITE, &OldProtect);

	// 2. ��ȡ��ԭ�е����ݽ��б��棬��ȡһ�ֽڵ�Դ���뵽�ϵ�ṹ����
	ReadProcessMemory(ProcessHandle, Address, &Int3Info.u.OldOpcode, 1, &Bytes);

	// 3. �� 0xCC д��Ŀ��λ��
	WriteProcessMemory(ProcessHandle, Address, "\xCC", 1, &Bytes);

	// 4. ��ԭ�ڴ�ı�������
	VirtualProtectEx(ProcessHandle, Address, 1, OldProtect, &OldProtect);

	// 5. ����ϵ㵽�б�
	BreakPointList.push_back(Int3Info);

	return false;
}
*/

// ��ȡ�û�������
VOID Debugger::GetCommand()
{
	cout << "����ָ����������룺t�� ������ϵ㣺bp�� ��Ӳ���ϵ㣺hbp�� ���鿴/�޸ļĴ�����reg�� ���鿴/�޸Ļ�ࣺasm��" <<"\n"
 	     << "          ���ڴ������men�� ��ջ������sta�� ���ڴ�ϵ㣺mbp�� ������������p�� �������ϵ㣺c�� ��ģ����Ϣ��m��"<<"\n"
		 << "          �����ز����plugin��" ;

	// ���ڱ���ָ����ַ���
	CHAR Command[20] = { 0 };
	// ��ȡ�û�����
	while (cin >> Command)
	{
		// ���ݲ�ͬ������ִ�в�ͬ�Ĳ���
		if (!strcmp(Command, "t"))
		{
			// �����ϵ㼴��������
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
			cout << "����Ҫ��������ϵ�ĵ�ַ: ";
			scanf_s("%x", &Address);
			BreakPoint::SetCcBreakPoint(ProcessHandle, (LPVOID)Address);
		
			//Ϊ�����öϵ�֮����ϵͳֱ���ܵ��ϵ㴦����Ҫ���öϵ�֮���һ��tf�ϵ㣬
			BreakPoint::SetTfBreakPoint(ThreadHandle);

			break;
		}
		else if (!strcmp(Command, "hbp"))
		{
			//���������¶ϵ�ĵ�ַ
			DWORD Address = 0;
			//�������նϵ�����
			DWORD Type = 0;
			//���ڽ��ն�������
			DWORD Len = 0;

			//����dr7�еı�־λ rw0~rw3 0����ʾִ�жϵ�   1����ʾд�ϵ�  3����ʾ��д�ϵ㣬��ȡָ������ִ�г���
			cout << "����ϵ����ͣ�0����ʾִ�жϵ�   1����ʾд�ϵ�  3����ʾ��д�ϵ㣩";
			cin >> Type;
			if (Type != 0 && Type != 1 && Type != 3)
			{
				cout << "����Ӳ���ϵ����ʹ��󣬶ϵ�����ʧ��";
			}
			else 
			{
				if (Type == 0)//�����ִ�жϵ㣬��ֱ��ʹ��Ĭ�ϲ��������¶Ϻ���
				{
					cout << "����Ҫ����Ӳ���ϵ�ĵ�ַ: ";
					scanf_s("%x", &Address);
					BreakPoint::SetHdBreakPoint(ThreadHandle, (LPVOID)Address);
				}
				else 
				{
					//len0~3 0��1�ֽڳ��ȣ�ִ�жϵ�ֻ����1�ֽڳ��ȣ� 1��2�ֽڳ��ȣ��ϵ��ַ����Ϊ2�ı��������϶��루��Ҫ�ں����ڲ����ϵ��ַ���룩
                    //2��8�ֽڳ���δ���峤��  3�����ֽڳ��ȣ��ϵ��ַ������4�ı������϶��루��Ҫ�ں����ڲ����ϵ��ַ���룩
					cout << "����ϵ��ַ��������";
					cin >> Len;
					if (Len != 0 && Len != 1 && Len != 2 && Len != 3)
					{
						cout << "����Ӳ���ϵ�������ȴ��󣬶ϵ�����ʧ��";
					}
					else
					{
						cout << "����Ҫ����Ӳ���ϵ�ĵ�ַ: ";
						scanf_s("%x", &Address);
						BreakPoint::SetHdBreakPoint(ThreadHandle, (LPVOID)Address,Type,Len);
					}
				}
			}
			//Ϊ�����öϵ�֮����ϵͳֱ���ܵ��ϵ㴦����Ҫ���öϵ�֮���һ��tf�ϵ㣬
			BreakPoint::SetTfBreakPoint(ThreadHandle);

			//����������������һ����Ӳ���ϵ�������tf�ϵ㣬�ᵼ�±����Գ������ִ�к�TF�ϵ㱻����������һ����Ӳ���ϵ㱻�޸�
			break;
		}
		else if (!strcmp(Command, "reg"))//��ʾ�����ļĴ���
		{
			ShowRegister(ThreadHandle);
		}
		else if (!strcmp(Command, "asm"))//��ʾ�����Ļ��
		{
			ChangeASM(ProcessHandle);
		}
		else if (!strcmp(Command, "mem"))//��ʾ�������ڴ�
		{
			ChangeMEM(ProcessHandle);
		}
		else if (!strcmp(Command, "sta"))//��ʾ������ջ
		{
			ChangeStack(ProcessHandle, ThreadHandle);
		}
		else if (!strcmp(Command, "mbp"))//�ڴ�ϵ�
		{
			DWORD Address = 0;
			cout << "����Ҫ�����ڴ�ϵ�ĵ�ַ: ";
			scanf_s("%x", &Address);
			BreakPoint::SetMemBreakPoint(ProcessHandle, (LPVOID)Address);
			//Ϊ�����öϵ�֮����ϵͳֱ���ܵ��ϵ㴦����Ҫ���öϵ�֮���һ��tf�ϵ㣬
			BreakPoint::SetTfBreakPoint(ThreadHandle);
			break;
		}
		else if (!strcmp(Command, "p"))//��������ָ��
		{
			// 0. �ṩ�ṹ�屣���̻߳�������Ҫָ������Ҫ��ȡ�ļĴ��������Ӷ����EFlags�Ĵ���
			CONTEXT Context = { CONTEXT_CONTROL };

			DWORD NestCodeAddress = 0;

			// 1. ��ȡ�̻߳���
			GetThreadContext(ThreadHandle, &Context);

			NestCodeAddress = Capstone::GetExceptionNextAddress(ProcessHandle, (LPVOID)Context.Eip, 10);

			BreakPoint::SetPassBreakPoint(ProcessHandle, ThreadHandle, NestCodeAddress);
		
			break;
		}
		else if (!strcmp(Command, "c"))//�����ϵ�
		{
			int RegNumber = 0;
			cout << "��ָ�������Ĵ��� ��EAX:1 EBX:2 ECX:3 EDX:4)";
			cin >> RegNumber;

			if (RegNumber != 1 && RegNumber != 2 && RegNumber != 3 && RegNumber != 4)
			{
				cout << "�����Ĵ���ָ�����������ϵ�����ʧ��";
				break;
			}
			else
			{
				ContionBpRegNumber = RegNumber;

				cout << "�����������Ĵ���ָ��ֵ";

				scanf_s("%x", &ContionBpNumber);

				cout << "�����������ϵ�ĵ�ֵַ";

				scanf_s("%x", &AddressOfContionBp);


				//std::cin.clear();  //����cin��־λ
				//std::cin.ignore(1024, '\n');

				//this->ContionBpFind

				///////////////////////////////////////////////////////
				////�ñ������ڼ��ָ���̵߳�eaxֵ�Ƿ�Ϊĳֵ
				//DWORD tid = 0;
				//// ����һ���߳�
				//CreateThread(NULL,/*�ں˶���İ�ȫ������*/
				//	0,/*���߳�ջ�Ĵ�С, 0��ʾĬ�ϴ�С*/
				//	ThreadProc,/*���̵߳Ļص�����*/
				//	this,/*���Ӳ���*/  //ע�����ｫdebuger��ָ�봮�������̣߳�ʹ����������ڵ���Debuger��ĺ���
				//	0,/*������־*/
				//	&tid);/*�������̵߳�id*/

				//�����̵߳õ��߳̾��
				//HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME,/*�����Ȩ��*/
					//FALSE,/*�Ƿ�̳�*/
					//tid);/*Ҫ�򿪵��߳�id*/
				/////////////////////////////////////////////////////
				BreakPoint::SetTfBreakPoint(ThreadHandle);
				break;
			}
		}
		else if (!strcmp(Command, "m"))
		{
			DWORD PID = GetProcessId(ProcessHandle);
			GetModule32and64(PID);
			BreakPoint::SetTfBreakPoint(ThreadHandle);
			break;
		}
		else if (!strcmp(Command, "plugin"))
		{
			GetModule();
			BreakPoint::SetTfBreakPoint(ThreadHandle);
			break;
		}
		else
		{
			cout << "�����ָ�����" << endl;
		}
	}
}



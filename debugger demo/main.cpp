#include "Debugger.h"

int main()
{
	Debugger debugger;

	// ��Ŀ����������ԻỰ
	debugger.open("demo.exe");

	// ��ʼ�ȴ������¼�������
	debugger.run();

	return 0;
}
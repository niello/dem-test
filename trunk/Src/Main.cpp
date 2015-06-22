#include <FactoryRegHelper.h>
#include <App/IPGApplication.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ForceFactoryRegistration();

	App::CIPGApplication AppInst;

	if (AppInst.Open()) 
	{
		while (AppInst.AdvanceFrame())
			;
		AppInst.Close();
	}

	return 0; 
}
//---------------------------------------------------------------------

#include <App/IPGApplication.h>
#include <FactoryRegHelper.h>

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

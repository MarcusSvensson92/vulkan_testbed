#include "App.h"

int main(int argc, char* argv[])
{
	App app;
	app.Initialize(1366, 768, "Vulkan Testbed");
	app.Run();
	app.Terminate();

    return 0;
}
#include "export_File.h"
#include "export_Plugin.h"
#include "exportPov_File.h"

static yafrayFileRender_t byfile;
static yafrayPluginRender_t byplugin;
static PovFileRender_t byPovfile;

yafrayRender_t *YAFBLEND = &byplugin;

extern "C"
{
	void YAF_switchPlugin() { YAFBLEND = &byplugin; }
	void YAF_switchFile() { YAFBLEND = &byfile; }
	void YAF_switchPovFile() {YAFBLEND=&byPovfile;}
	int YAF_exportScene(Render* re) { return (int)YAFBLEND->exportScene(re); }
}

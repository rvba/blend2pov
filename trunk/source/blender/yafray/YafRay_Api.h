#ifndef __YAFRAY_API_H
#define __YAFRAY_API_H

/* C interface for Blender */
#ifdef __cplusplus
extern "C" {
#endif
	void YAF_switchPlugin();
	void YAF_switchFile();
	void YAF_switchPovFile();
	int YAF_exportScene(Render* re);
#ifdef __cplusplus
}
#endif

#endif

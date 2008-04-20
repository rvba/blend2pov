//rcruiz
#ifndef __EXPORTPOV_FILE_H
#define __EXPORTPOV_FILE_H

#include"yafray_Render.h"
#include "DNA_text_types.h"

class PovFileRender_t : public yafrayRender_t
{
	public:
		virtual ~PovFileRender_t() {}

	protected:
		std::string imgout;
		std::ofstream xmlfile;
		std::string xmlpath;
		std::ostringstream ostr;

		void displayImage();
		bool executeYafray(const std::string &xmlpath);
		virtual void writeTextures();
		virtual void writeShader(const std::string &shader_name, Material* matr, const std::string &facetexname="");
		virtual void writeMaterialsAndModulators();
		virtual void writeObject(Object* obj,ObjectRen *obr, const std::vector<VlakRen*> &VLR_list, const float obmat[4][4]);
		virtual void writeAllObjects();
		virtual void writeAreaLamp(LampRen* lamp, int num, float iview[4][4]);
		virtual void writeLamps();
		virtual void writeCamera();
		virtual void writeHemilight();
		virtual void writePathlight();
		virtual bool writeWorld();
		virtual bool writeRender();
		virtual bool initExport();
		virtual bool finishExport();
		virtual void writeObjectPov(Object* obj, ObjectRen *obr, const std::vector<VlakRen*> &VLR_list, const float obmat[4][4]);//rcruiz
		virtual bool isToPov();
		virtual void MatBoundbox(Object* obj);
		std::ofstream povM;
		std::ofstream povMat;
		std::ofstream pov;
		std::ofstream povIni;
		int tipRadPov;
		float defAmbPov;
		bool useMegaPov;
		bool materialneutro;
		Text *PovMat;
        	Text *PovCam;
       		Text *PovGlobalRad;
        	Text *PovGlobalPhot;
        	Text *PovIniCommand;
        	Text *PovInit;
        	std::string yafExpPath;
		/*milo*/
		/*sun*/
		virtual void writesun(LampRen* lamp,float iview[4][4]);
		/*blur*/
		virtual void writeblur();
		/*pattern map*/

		std::ostringstream in_pattern_map;
		std::string out_pattern_map;

		std::ostringstream in_material_bright;
		std::string out_material_bright;

		std::ostringstream in_material_dark;
		std::string out_material_dark;

		/*temp*/
		
		std::ostringstream temp_pattern_bright;
		std::ostringstream temp_pattern_dark;
		
		/* mult */

		float bright_mult ;


	
};

#endif

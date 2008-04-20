//rcruiz&karpov
/*
TODO
*/

#include "BLI_arithb.h"

#include "BKE_main.h"



/*#ifdef __cplusplus

extern "C" {

#endif

#include "BSE_headerbuttons.h"

void update_for_newframe();

#ifdef __cplusplus

}

#endif*/


#include "exportPov_File.h"
#include <math.h>

using namespace std;


static string command_path = "";


#ifdef WIN32

#define WIN32_SKIP_HKEY_PROTECTION
#ifndef FILE_MAXDIR
#define FILE_MAXDIR  160

#endif

#ifndef FILE_MAXFILE
#define FILE_MAXFILE 80

#endif
/* 
 * createDira
 * addDrive
 * findpathPov
 * unixPovPath
 * elimChars
 * getUserMat
 * strLow
 * :initExport
 * :writeRender
 * :finishExport
 * :displayImage
 * adjustPath
 * :writeTextures
 * :writeShader
 * :writeAllObjects
 * :writeAreaLamp
 * p:writeSun
 * :writeLamps
 * :writeCamera
 * :executeYafray
 * struct normAux
 * struct uvAux
 * struct fcAux
 * :MatBoundBox
 * :writeObjectPov
 * ::isToPov
 * ::writeMaterialsModulators
 * -::writeObjects
 *  -writePathLight
 */

/*	CREATE DIR ***********************************************************************************************	*/

static int createDir(char* name)

{

	if (BLI_exists(name))

		return 2;	//exists

	if (CreateDirectory((LPCTSTR)(name), NULL)) {

		cout << "Directory: " << name << " created\n";

		return 1;	// created

	}

	else	{

		cout << "Could not create directory: " << name << endl;

		return 0;	// fail

	}

}

extern "C" { extern char bprogname[]; }

// add drive character if not in path string, using blender executable location as reference

/* ADD DRIVE *************************************************************************************************	*/


static void addDrive(string &path)

{

	int sp = path.find_first_of(":");

	if (sp==-1) {

		string blpath = bprogname;

		sp = blpath.find_first_of(":");

		if (sp!=-1) path = blpath.substr(0, sp+1) + path;

	}

}

/*	FIND PATH POV ****************************************************************************************************************	*/

static string find_pathPov()

{

	HKEY	hkey;

	DWORD dwType, dwSize;



	if (RegOpenKeyEx(HKEY_CLASSES_ROOT,"POV-Ray.Scene\\shell\\Open\\command",0,KEY_READ,&hkey)==ERROR_SUCCESS)

	{

		dwType = REG_EXPAND_SZ;

	 	dwSize = MAX_PATH;

		DWORD dwStat;



		char *pInstallDir=new char[MAX_PATH];



  		dwStat=RegQueryValueEx(hkey, TEXT(""),

			NULL, NULL,(LPBYTE)pInstallDir, &dwSize);



		if (dwStat == NO_ERROR)

		{

			string res=pInstallDir;

			string tmp;

			for (unsigned int i=0;i<res.length()-24;i++)

                tmp=tmp+res[i];

			delete [] pInstallDir;



			return tmp;

		}

		else

			cout << "Couldn't READ \'InstallDir\' value. Is Pov-ray correctly installed?\n";

		delete [] pInstallDir;



		RegCloseKey(hkey);

	}

	else

		cout << "Couldn't FIND registry key for Pov-ray, is it installed?\n";



	return string("");



}



#else



#include <sys/stat.h>

#include <sys/types.h>

#include <sys/wait.h>

#include <signal.h>

#include <stdlib.h>

#include <unistd.h>


/*	UNIX POV PATH ****************************************************************************************************************	*/


static string unixPovPath()

{

	static char *alternative[]=

	{

		"/usr/local/bin/",

		"/usr/bin/",

		"/bin/",

		NULL

	};

    string fp;

	for(int i=0;alternative[i]!=NULL;++i)

	{

        fp=string(alternative[i])+"povray";

		struct stat st;

		if(stat(fp.c_str(),&st)<0) continue;

		if(st.st_mode&S_IXOTH) return alternative[i];

	}

	return "";

}



#endif

/*	ELIM CHARS ******************************************************************************************************************	*/


string elimChars(string str)

{

    for(unsigned int i=0;i<str.length();i++)

        if (str[i]== '.' || str[i]== ' ' || str[i]== '-' ||str[i]== '+' ||str[i]== '/' ||str[i]== '*' ||str[i]== '&')

            str[i]='_';

    return str;



}

/* GET USER MAT **************************************************************************************************************	*/


string getUserMat(string mat,Text* PovMat,bool add){

    string umat="";

    char busq='*';

    if (add) busq='+';

    TextLine *line=(TextLine *)PovMat->lines.first;

    if ((PovMat->nlines!=1) || (line->len!=0)){

        bool grab = false;

        while (line){

            if ((line->line[0]=='+')||(line->line[0]=='*')){

                string tmp1="",tmp2="";

                for (unsigned int i=2;i<mat.length();i++)

                    tmp1=tmp1+mat[i];

                for (int i=1;i<line->len;i++)

                    tmp2=tmp2+line->line[i];

                if ((line->line[0]==busq)&&(tmp1==tmp2))//(busq+"MA"+line->line)==mat)

                    grab=true;

                else

                    grab = false;

            }

            else{

                if (grab)

                    umat=umat+line->line+"\n";

            }

            line=line->next;

        }

    }

    return umat;

}


/* STRLOW **************************************************************************************************************	*/

string strLow(string str){

    string tmp;

    int chr1;

    char chr2;

    for(unsigned int i=0;i<str.length();i++)

        if ((str[i]>=65)&&(str[i]<=90)){

            chr1=str[i]+32;

            chr2=chr1;

            tmp=tmp+chr2;

        }

        else

            tmp=tmp+str[i];

    return tmp;

}


/*	INIT EXPORT ********************************************************************************************	*/


bool PovFileRender_t::initExport()

{

	/*milo*/
	/*ini*/

	std::string povINI;
	std::stringstream out;
	out << re->r.povINI;
	povINI = out.str();

    if (isToPov()){

        useMegaPov=false;

        cout<<" Exporting. . ."<<endl;

    }

    else

        return false;

	xmlpath = "";

	bool dir_failed = false;

	// try the user setting setting first, export dir must be set and exist

	if (strlen(U.yfexportdir)==0)

	{

		cout << "No export directory set in user defaults!" << endl;

		char* temp = getenv("TEMP");

		// if no envar, use /tmp

		//xmlpath = temp ? temp : "/tmp";
		xmlpath = temp ? temp : "/home/milovann/6-TEMP";

		cout << "Will try TEMP instead: " << xmlpath << endl;

		// no fail here, but might fail when opening file...

	}

	else

	{

		// check if it exists

		if (!BLI_exists(U.yfexportdir)) {

			cout << "YafRay temporary xml export directory:\n" << U.yfexportdir << "\ndoes not exist!\n";

#ifdef WIN32

			// try to create it

			cout << "Trying to create...\n";

			if (createDir(U.yfexportdir)==0) dir_failed=true; else dir_failed=false;

#else

			dir_failed = true;

#endif

		}

		xmlpath = U.yfexportdir;

#ifdef WIN32

		// have to add drive char here too, in case win user still wants to set path him/herself

		addDrive(xmlpath);

#endif

	}



#ifdef WIN32

	// for windows try to get the path to the yafray binary from the registry, only done once

	command_path="";

	if (command_path=="")

	{

		char path[FILE_MAXDIR+FILE_MAXFILE];

		string yafray_path;

        yafray_path = find_pathPov();

		if (yafray_path=="")

		{

			// error already printed in find_path()

			clearAll();

			return false;

		}

		GetShortPathName((LPCTSTR)(yafray_path.c_str()), path, FILE_MAXDIR+FILE_MAXFILE);

		command_path = string(path) + "\\";

        cout << "Pov-Ray found!" << endl;

	}

	// if no export dir set, or could not create, try to create one in the yafray dir, unless it already exists

	if (dir_failed)

	{

		string ybdir = command_path + "YBtest";

		if (createDir(const_cast<char*>(ybdir.c_str()))==0) dir_failed=true; else dir_failed=false;

		xmlpath = ybdir;

	}

#else

	if (command_path=="")

	{

		command_path = unixPovPath();

	}

#endif



	// for all

	if (dir_failed) return false;



#ifdef WIN32

	string DLM = "\\";

#else

	string DLM = "/";

#endif


	// remove trailing slash if needed


	if (xmlpath.find_last_of(DLM)!=(xmlpath.length()-1)) xmlpath += DLM;



	/*milo*/
	/*ini*/
    imgout = xmlpath + "Exp" + povINI + ".tga";

    string tmp;

    string frame;

    /* no exec -> frame number */

    if (G.scene->r.povmode & R_POV_EXECUTE){//(PreviewPov==0){

           ostr.str("");

           ostr<<G.scene->r.cfra;

           frame=ostr.str();

           ostr.str("");

    }

    else frame="";

    if (!(G.scene->r.povmode & R_POV_NOT_SEND_MESH)){


        tmp =xmlpath+"ExpM"+frame+povINI+".pov";

        povM.open(tmp.c_str());

        povM<<"//*pov"<<endl;

    }

    tmp =xmlpath+"ExpMat"+frame+povINI+".pov";

    povMat.open(tmp.c_str());

    povMat<<"//*pov"<<endl;

    tmp =xmlpath+"Exp"+frame+povINI+".pov";

    pov.open(tmp.c_str());

    pov<<"//*pov"<<endl;

    tmp =xmlpath+"Exp"+frame+povINI+".ini";

    yafExpPath=xmlpath;

    povIni.open(tmp.c_str());

    povIni<<";_pov"<<endl;

    povIni<<"Input_File_Name="<<xmlpath<<"Exp"<<frame<<povINI<<".pov\n+FT"<<endl;

    povIni<<"Output_File_Name="<<xmlpath<<"Exp"<<frame<<povINI<<".tga"<<endl;

    povIni<<"+GF"<<"Exp.err"<<endl;

    povIni<<"+MB3"<<endl;
    // milo
    // Library paths
    povIni<<"+L/home/milovann/2-DATA/2-POVRAY/FILES/LIGHTSYS"<<endl;
    povIni<<"+L/home/milovann/2-DATA/2-POVRAY/FILES/GENERIC"<<endl;
    povIni<<"+L/home/milovann/2-DATA/2-POVRAY/FILES/HDRI"<<endl;
    povIni<<"+L/home/milovann/2-DATA/2-POVRAY/FILES/MACROS"<<endl;
    povIni<<"+L/home/milovann/2-DATA/2-POVRAY/FILES/MATERIALS"<<endl;
    povIni<<"+L/home/milovann/2-DATA/2-POVRAY/FILES/MATERIALS/ISOWOOD"<<endl;

	//
	// BORDER
	// border rctf*  DNA_scene_types.h
	// buttons_scene.c
	// editview.c
	// drawview.c
	if (G.scene->r.mode & R_POVBORDER)
	{

		rctf border = G.scene->r.povborder;

		povIni<<"Start_Column = "<<border.xmin<<endl;
		povIni<<"End_Column = "<<border.xmax<<endl;
		povIni<<"Start_Row = "<<1-border.ymax<<endl;
		povIni<<"End_Row = "<<1-border.ymin<<endl;
	}

	/*milo*/
    pov<<"#declare curframe="<<G.scene->r.cfra<<";\n#include \"colors.inc\"\n#include \""<<xmlpath<<"ExpMat"<<frame<<povINI<<".pov\"\n#include \""<<xmlpath<<"ExpM"<<frame<<povINI<<".pov\"\n#include \"rad_def.inc\""<<endl;

    TextLine *line=(TextLine *)PovInit->lines.first;

    while (line){

           if (line->len>7)

               if ((line->line[0]=='m') && (line->line[1]=='e') && (line->line[2]=='g') && (line->line[3]=='a') && (line->line[4]=='p') && (line->line[5]=='o') && (line->line[6]=='v')){

                   useMegaPov=true;

                   pov<<"#version unofficial "<<line->line<<";"<<endl;

               }

               else

                   pov<<line->line<<endl;

           else

               pov<<line->line<<endl;

           line=line->next;

    }

    line=(TextLine *)PovIniCommand->lines.first;

    while (line){

            povIni<<line->line<<endl;

            line=line->next;

    }



	ostr << setiosflags(ios::showpoint | ios::fixed);

	return true;

}

/*	WRITE RENDER ****************************************************************************************************************	*/


bool PovFileRender_t::writeRender()

{

    if (re->r.YF_exposure!=0.0){

        //pov <<"#version unofficial megapov 1.1;"<<endl;
        pov <<"#version unofficial megapov 1.2;"<<endl;

        useMegaPov=true;

    }

    pov<<"global_settings { max_trace_level "<<re->r.YF_raydepth<<" assumed_gamma "<<re->r.YF_gamma;

    if (re->r.YF_exposure!=0.0) pov<<" exposure "<<re->r.YF_exposure;

    if (G.scene->r.povmode & R_POV_PHOTONS){

	        TextLine *line=(TextLine *)PovGlobalPhot->lines.first;

	        pov<<" photons { ";

	        if ((PovGlobalPhot->nlines==1) && (line->len==0))

                pov<<"spacing 0.03";

            else{



                while (line){

                    pov<<line->line<<endl;

                    line=line->next;

                }

            }

            if (G.scene->r.povmode & R_POV_SAVE_RAD_PH)//(SalvaRadioPhotonsPov==1)

                pov<<" save_file \""<<yafExpPath<<"Exp.ph\"";

            else

                if (G.scene->r.povmode & R_POV_LOAD_RAD_PH)//(CargarRadioPhotonsPov==1)

                    pov<<" load_file \""<<yafExpPath<<"Exp.ph\"";

            pov<<"}";

    }

    if (tipRadPov!=0){

	        TextLine *line=(TextLine *)PovGlobalRad->lines.first;

	        pov<<" radiosity { ";

	        if ((PovGlobalRad->nlines==1) && (line->len==0)){

	            pov<<"Rad_Settings(";

	            if (tipRadPov==1)

                    pov<<"Radiosity_Default"<<"/n";

                else if (tipRadPov==2)

                    pov<<"Radiosity_Fast";

                else if (tipRadPov==3)

                    pov<<"Radiosity_Normal";

                else if (tipRadPov==4)

                    pov<<"Radiosity_2Bounce";

                else if (tipRadPov==5)

                    pov<<"Radiosity_Final";

                else if (tipRadPov==6)

                    pov<<"Radiosity_OutdoorLQ";

                else if (tipRadPov==7)

                    pov<<"Radiosity_OutdoorHQ";

                else if (tipRadPov==8)

                    pov<<"Radiosity_OutdoorLight";

                else if (tipRadPov==9)

                    pov<<"Radiosity_IndoorLQ";

                else if (tipRadPov==10)

                    pov<<"Radiosity_IndoorHQ";

                pov<<", off, off)";

	        }

            else{

                cout <<line->len<<endl;

                cout <<PovGlobalRad->nlines<<endl;

                while (line){

                    pov<<line->line<<endl;

                    line=line->next;

                }

            }

            if (G.scene->r.povmode & R_POV_SAVE_RAD_PH)

                pov<<" save_file \""<<yafExpPath<<"Exp.rad\"";

            else

               if (G.scene->r.povmode & R_POV_LOAD_RAD_PH)

                   pov<<" load_file \""<<yafExpPath<<"Exp.rad\" always_sample off pretrace_start 1 pretrace_end 1";

            pov<<"}";

    }

    pov<<"}"<<endl;



    if (materialneutro)

    povIni<<"Antialias=Off"<<endl;

    else{

        if(re->r.YF_AA) {

            povIni<<"Antialias=On\n"<<"Antialias_Depth="<< re->r.YF_AApasses<<"\nAntialias_Threshold="<<re->r.YF_AAthreshold<<endl;

        }

        else {

            if ((re->r.mode & R_OSA) && (re->r.osa)) {

                int passes=(re->r.osa%4)==0 ? re->r.osa/4 : 1;

                povIni<<"Antialias=On\n"<<"Antialias_Depth="<< passes<<"\nAntialias_Threshold=0.05"<<endl;

            }

            else

                povIni<<"Antialias=Off"<<endl;

        }

    }



	if (re->r.planes==R_PLANES32)  povIni<<"+UA"<<endl;



	return true;

}

/*	FINISH EXPORT*******************************************************************************************************************	*/

bool PovFileRender_t::finishExport()

{

    if (!(G.scene->r.povmode & R_POV_NOT_SEND_MESH))

        povM.close();

    povMat.close();

    pov.close();

    povIni.close();

    cout<<"\nBlend2Pov output Done . (RCRuiz test for Blend2Pov 0.0.6j)\n"<<endl;

    if (!(G.scene->r.povmode & R_POV_EXECUTE))//(PreviewPov==1)

        if (executeYafray(xmlpath)){

            string line;

            string tmp =xmlpath+"Exp.err";

            ifstream myfile (tmp.c_str());

            if (myfile.is_open()){

                bool NOerr=true;

                getline (myfile,line);

                while (! myfile.eof() ){

                    if (NOerr){

                        //error("Errors on PovRay");

                        cout << "Errors on PovRay :\n"<<endl;

                        NOerr=false;

                    }

                    cout << line << endl;

                    getline (myfile,line);

                }

                myfile.close();

                if (NOerr){

                    displayImage();

                    cout<<"\nBlend2Pov execution Done . (RCRuiz test for Blend2Pov 0.0.6j)\n"<<endl;

                }

            }else{

                displayImage();

                cout<<"\nBlend2Pov execution Done . (RCRuiz test for Blend2Pov 0.0.6j)\n"<<endl;

            }

        } else

        {

            cout << "Could not execute PovRay. Is it in path?" << endl;

            return false;

        }

    return true;

}

/*	DISPLAY IMAGE ********************************************************************************************************************	*/


// displays the image rendered with xml export

// Now loads rendered image into blender renderbuf.

void PovFileRender_t::displayImage()

{

	// although it is possible to load the image using blender,

	// maybe it is best to just do a read here, for now the yafray output is always a raw tga anyway



	FILE* fp = fopen(imgout.c_str(), "rb");

	if (fp==NULL) {

		cout << "displayImage(): Could not open image file\n";

		return;

	}



	unsigned char header[18];

	fread(&header, 1, 18, fp);

	unsigned short width = (unsigned short)(header[12] + (header[13]<<8));

	unsigned short height = (unsigned short)(header[14] + (header[15]<<8));

	// don't do anything if resolution doesn't match that of rectot

	if ((width!=re->rectx) || (height!=re->recty)) {

		fclose(fp);

		fp = NULL;

		return;

	}

	unsigned char byte_per_pix = (unsigned char)(header[16]>>3);

	// read past any id (none in this case though)

	unsigned int idlen = (unsigned int)header[0];

	if (idlen) fseek(fp, idlen, SEEK_CUR);



	/* XXX how to get the image from Blender and write to it. This call doesn't allow to change buffer rects */

	RenderResult rres;

	RE_GetResultImage(re, &rres);

	// rres.rectx, rres.recty is width/height

	// rres.rectf is float buffer, scanlines starting in bottom

	// rres.rectz is zbuffer, available when associated pass is set



	// read data directly into buffer, picture is upside down

	const float btf = 1.f/255.f;

	for (unsigned short y=0;y<height;y++) {

		float* bpt = (float*)rres.rectf + ((((height-1)-y)*width)<<2);

		for (unsigned short x=0;x<width;x++) {

			bpt[2] = ((float)fgetc(fp) * btf);

			bpt[1] = ((float)fgetc(fp) * btf);

			bpt[0] = ((float)fgetc(fp) * btf);

			if (byte_per_pix==4)

				bpt[3] = ((float)fgetc(fp) * btf);

			else

				bpt[3] = 1.f;

			bpt += 4;

		}

	}



	fclose(fp);

	fp = NULL;

}



#ifdef WIN32

#define MAXPATHLEN MAX_PATH

#else

#include <sys/param.h>

#endif
/*	ADJUST PATH *******************************************************************************************************************	*/


static void adjustPath(string &path)

{

	// if relative, expand to full path

	char cpath[MAXPATHLEN];

	strcpy(cpath, path.c_str());

	BLI_convertstringcode(cpath, G.sce, 0);

	path = cpath;

#ifdef WIN32

	// add drive char if not there

	addDrive(path);

#endif

}


/*	WRITE TEXTURE	************************************************************************************************************	*/


void PovFileRender_t::writeTextures()

{

	// used to keep track of images already written

	// (to avoid duplicates if also in imagetex for material TexFace texture)

	set<Image*> dupimg;

	povMat<<"//MAPS"<<endl;

	for (map<string, MTex*>::const_iterator blendtex=used_textures.begin();

						blendtex!=used_textures.end();++blendtex) {

		MTex* mtex = blendtex->second;

		Tex* tex = mtex->tex;



		float nsz = tex->noisesize;

		if (nsz!=0.f) nsz=1.f/nsz;



		string ts, hardnoise=(tex->noisetype==TEX_NOISESOFT) ? "off" : "on";

    if (tex->flag & TEX_COLORBAND) {

              ColorBand* cb = tex->coba;

              if (cb) {

                  povMat<<"#declare "<< elimChars(blendtex->first) << "_coba = color_map {";

                  for (int i=0;i<cb->tot;i++)

                      povMat<<"["<< cb->data[i].pos <<" color rgb <"<<cb->data[i].r<<","<<cb->data[i].g<<","<<cb->data[i].b<<","<<cb->data[i].a<<">]";

                  povMat<<"}"<<endl;

              }

    }

 	switch (tex->type) {
			/* CLOUDS */

			case TEX_CLOUDS: {

			    povMat<<"#declare "<<elimChars(blendtex->first)<<"_pigm = pigment { bozo";

			    if (tex->flag & TEX_COLORBAND)

                    povMat<<" color_map {"<<elimChars(blendtex->first) << "_coba}";

                else

                    povMat<<" color_map {[0.25 color rgb <1,1,1,"<<mtex->colfac<<">][0.65 color rgb <"<<mtex->r<<","<<mtex->g<<","<<mtex->b<<">]}";

                povMat<<" turbulence "<<nsz;

                povMat<<" scale <"<<mtex->size[0]<<","<<mtex->size[2]<<","<<mtex->size[1]<<"> translate <"<<mtex->ofs[2]<<","<<mtex->ofs[1]<<","<<mtex->ofs[0]<<"> }"<<endl;

				break;

			}
			/* milo */
			/* IMAGE */

			case TEX_IMAGE: {

				Image* ima = tex->ima;

				if (ima) {

					dupimg.insert(ima);

					string texpath(ima->name);

					adjustPath(texpath);

					string extens="";

					bool encontreExt=false;

					for (int i=texpath.length()-1;i!=0;i--)

                        if (texpath[i]!='.')

                            extens=texpath[i]+extens;

                        else{
                            extens=strLow(extens);

                            if ((extens=="gif") || (extens=="tiff") || (extens=="tif") || (extens=="bmp") || (extens=="tga") || (extens=="iff") || (extens=="ppm") || (extens=="png") || (extens=="jpeg") || (extens=="jpg") || (extens=="hdr") || (extens=="hdri")){

                                i=1;

                                encontreExt=true;

                            }

                            else
                                extens="";
                        }

                    if (encontreExt){

                        if (extens=="bmp") extens="sys";
                        if (extens=="jpg") extens="jpeg";
                        if (extens=="hdri") extens="hdr";
                        if (extens=="hdr") if (!useMegaPov)  povMat <<"#version unofficial megapov 1.1;"<<endl;

						/*milo */
						/* image_pattern */
								if (mtex->mapto & MAP_SPEC)
								{
									in_pattern_map<<""<<extens<<" \""<<texpath<<"\""<<endl;
								}

                        povMat<<"#declare "<<elimChars(blendtex->first)<<"_pigm = pigment { image_map{"<<extens<<" \""<<texpath<<"\" transmit all "<<1-mtex->colfac;

                        if (tex->imaflag & TEX_INTERPOL) povMat<<" interpolate 2 ";
                        if (extens=="hdr") {
                            useMegaPov=true;
                            povMat<<" once map_type 7 ";
                        }
                        else{

                            if (!(tex->extend & TEX_CLIP)) povMat<<" once ";
                            if (mtex->mapping & MTEX_SPHERE) povMat<<" map_type 1 ";
                            if (mtex->mapping & MTEX_CUBE) povMat<<" map_type 5 ";
                            if (mtex->mapping & MTEX_TUBE) povMat<<" map_type 2 ";
                        }

                        povMat<<"}";
                        povMat<<" scale <"<<mtex->size[0]<<","<<mtex->size[2]<<","<<mtex->size[1]<<"> translate <"<<mtex->ofs[2]<<","<<mtex->ofs[1]<<","<<mtex->ofs[0]<<"> }"<<endl;

                    }

                    else

                        povMat<<"#declare "<<elimChars(blendtex->first)<<"_pigm = pigment { rgb 1} //Extencion para imagen no encontrada \""<<texpath<<"\""<<endl;

				}

				break;

			}
			/* BLEND */

			case TEX_BLEND: {

			    povMat<<"#declare "<<elimChars(blendtex->first)<<"_pigm = pigment { gradient";

			    if (tex->flag & TEX_FLIPBLEND)

                    povMat<<" y";

                else

                    povMat<<" x";

			    if (tex->flag & TEX_COLORBAND)

                    povMat<<" color_map {"<<elimChars(blendtex->first) << "_coba}";

                else

                    povMat<<" color_map {[0.25 color rgb <1,1,1,"<<mtex->colfac<<">][0.65 color rgb <"<<mtex->r<<","<<mtex->g<<","<<mtex->b<<">]}";

                povMat<<" scale <"<<mtex->size[0]<<","<<mtex->size[2]<<","<<mtex->size[1]<<"> translate <"<<mtex->ofs[2]<<","<<mtex->ofs[1]<<","<<mtex->ofs[0]<<"> }"<<endl;

				break;

			}
			/* NOISE */

			case TEX_NOISE: {

			    povMat<<"#declare "<<elimChars(blendtex->first)<<"_pigm = pigment { granite";

			    if (tex->flag & TEX_COLORBAND)

                    povMat<<" color_map {"<<elimChars(blendtex->first) << "_coba}";

                else

                    povMat<<" color_map {[0.25 color rgb <1,1,1,"<<mtex->colfac<<">][0.65 color rgb <"<<mtex->r<<","<<mtex->g<<","<<mtex->b<<">]}";

                povMat<<" scale <"<<mtex->size[0]<<","<<mtex->size[2]<<","<<mtex->size[1]<<"> translate <"<<mtex->ofs[2]<<","<<mtex->ofs[1]<<","<<mtex->ofs[0]<<"> }"<<endl;

				break;

			}
			/*WOOD */

			case TEX_WOOD: {

			    float turb = (tex->stype<2) ? 0.0 : tex->turbul;

			    povMat<<"#declare "<<elimChars(blendtex->first)<<"_pigm = pigment { wood";

			    if (tex->flag & TEX_COLORBAND)

                    povMat<<" color_map {"<<elimChars(blendtex->first) << "_coba}";

                else

                    povMat<<" color_map {[0.25 color rgb <1,1,1,"<<mtex->colfac<<">][0.65 color rgb <"<<mtex->r<<","<<mtex->g<<","<<mtex->b<<">]}";

			    povMat<<" turbulence "<<turb<<" scale "<<nsz*0.1;

			    ts = "sine_wave";

				if (tex->noisebasis2==1) ts="scallop_wav"; else if (tex->noisebasis2==2) ts="triangle_wave";

				povMat<<"  lambda 3.5 octaves 5 "<<ts;

                povMat<<" scale <"<<mtex->size[0]<<","<<mtex->size[2]<<","<<mtex->size[1]<<"> translate <"<<mtex->ofs[2]<<","<<mtex->ofs[1]<<","<<mtex->ofs[0]<<"> }"<<endl;

				break;

			}
			/* MARBLE */

			case TEX_MARBLE: {

			    float turb = (tex->stype<2) ? 0.0 : tex->turbul;

			    povMat<<"#declare "<<elimChars(blendtex->first)<<"_pigm = pigment { marble";

			    if (tex->flag & TEX_COLORBAND)

                    povMat<<" color_map {"<<elimChars(blendtex->first) << "_coba}";

                else

                    povMat<<" color_map {[0.25 color rgb <1,1,1,"<<mtex->colfac<<">][0.65 color rgb <"<<mtex->r<<","<<mtex->g<<","<<mtex->b<<">]}";

			    povMat<<" turbulence "<<turb<<" scale "<<nsz*0.1;

			    ts = "sine_wave";

				if (tex->noisebasis2==1) ts="scallop_wav"; else if (tex->noisebasis2==2) ts="triangle_wave";

				povMat<<"  lambda 3.5 octaves 5 "<<ts;

                povMat<<" scale <"<<mtex->size[0]<<","<<mtex->size[2]<<","<<mtex->size[1]<<"> translate <"<<mtex->ofs[2]<<","<<mtex->ofs[1]<<","<<mtex->ofs[0]<<"> }"<<endl;

				break;

			}

			default:

			    povMat<<"#declare "<<elimChars(blendtex->first)<<"_pigm = pigment { rgb 1}"<<endl;

 		}

 }

}

/*	WRITE SHADER	********************************************************************************************************************************************** */


void PovFileRender_t::writeShader(const string &shader_name, Material* matr, const string &facetexname)

	{

	// if material has ramps, export colorbands first

	/*milo*/
	int image_pattern=0;
	float bright_mult=0.5;

	float bg_mult = (re->r.GImethod==0) ? 1 : re->r.GIpower;

	string finish="",interior="",normalGeneral="";
	string finish_bright="",finish_dark="";

    	ostr.str("");

	if (!materialneutro)

	for (int m2=0;m2<MAX_MTEX;m2++)
		{
		

		if (matr->septex & (1<<m2)) continue;// all active channels

		MTex* mtex = matr->mtex[m2];
		

		if (mtex==NULL) continue;

		Tex* tex = mtex->tex;

		if (tex==NULL) continue;

		map<string, MTex*>::const_iterator mtexL = used_textures.find(string(tex->id.name));

		if (mtexL!=used_textures.end())
			{
			if (mtex->mapto & MAP_SPEC)	image_pattern=1; /*pattern_map  DNA_material_types.h */
			/* normal */

                	if ((mtex->mapto & MAP_NORM) || (mtex->maptoneg & MAP_NORM))
				{

                		if (normalGeneral=="")normalGeneral=" normal {average normal_map{";

                    		float nf = mtex->norfac;

                    		if (tex->type!=TEX_STUCCI) nf *= -1.f;

                    		if (mtex->maptoneg & MAP_NORM) nf *= -1.f;

                    		ostr<<" [ "<<mtex->colfac<<" pigment_pattern {"<<elimChars(mtexL->first)<<"_pigm} bump_size "<<(-nf)<<" ]";

                		}

			}


		}


	if (normalGeneral!="")

        normalGeneral=normalGeneral+ostr.str()+" }}";
	povMat<<"// "<<elimChars(shader_name)<<endl;

    	povMat<<"#declare "<<elimChars(shader_name)<<"_tex ="<<endl;
	/* spe */
	if (image_pattern)
		{

		in_material_bright<<"#declare "<<elimChars(shader_name)<<"_bright =";
		in_material_dark<<"#declare "<<elimChars(shader_name)<<"_dark =";
		}
	/* neutro */

    	if (materialneutro)
		{
		povMat<<"//DEFAULT"<<endl;

        	povMat<<" texture { pigment { color rgb <1,1,1,"<<1.0-matr->alpha<<">}";
		}
	/* if no map : object color */

    	/*else*/
	/*MTex* mtex = matr->mtex[0];*/
	/*if (mtex==NULL)*/
	/*here...*/
	/*if ( (mtex==NULL )||( (mtex->mapto & MAP_COL)==0))*/
	else
		{
			/*blur*/
			if ( matr->pov_mat_blur_toggle) {
				povMat<<"// blured reflection\n";
				povMat<<"texture {\n";
				povMat<<"#declare BlurAmount="<<matr->pov_mat_blur_amount<<";\n";
				povMat<<"#declare BlurSamples="<<matr->pov_mat_blur_samples<<";\n";
				
				povMat<<"average texture_map {\n";
				povMat<<"#declare Ind=0;\n";
				povMat<<"#declare S= seed(0);\n";
				povMat<<"#while ( Ind<BlurSamples )\n";
				povMat<<"[1 pigment { color rgb <"<< matr->r <<","<< matr->g <<","<< matr->b <<","<<1.0-matr->alpha<<">}\n";
				}
			else {
        	povMat<<" texture { pigment { color rgb <"<< matr->r <<","<< matr->g <<","<< matr->b <<","<<1.0-matr->alpha<<">}"<<normalGeneral<<"}"<<endl;
			}
		}
	
	/* finish*/

    	ostr.str("");
	temp_pattern_bright.str("");
	temp_pattern_dark.str("");
	

    	ostr<<" finish { diffuse "<<matr->ref<<" specular "<<matr->spec<<" ambient ";
	/* bright / dark */
	temp_pattern_bright<<" finish { diffuse "<<matr->ref+bright_mult<<" specular "<<matr->spec+bright_mult<<" ambient ";
	temp_pattern_dark<<" finish { diffuse "<<matr->ref-bright_mult<<" specular "<<matr->spec-bright_mult<<" ambient ";


	if ((matr->emit * bg_mult)==0.0) ostr<<defAmbPov;

	else ostr<<(matr->emit * bg_mult)*10.0;


    	ostr<<" roughness 1/"<<matr->har;

	if (matr->spec_shader==MA_SPEC_BLINN) ostr<<" phong 0.9 phong_size "<<matr->refrac*10.0;

	if (matr->mode & MA_RAYMIRROR)

            ostr<<" conserve_energy reflection rgb <"<<matr->mirr*matr->ray_mirror <<","<<matr->mirg*matr->ray_mirror <<","<<matr->mirb*matr->ray_mirror <<">";


    	ostr<<"}";

    	finish=ostr.str();	

	/*blur*/
	if (matr->pov_mat_blur_toggle){
		povMat<<finish<<endl;
		povMat<<"normal\n";
		povMat<<"{ bumps BlurAmount\n";
		povMat<<"translate <rand(S),rand(S),rand(S)>*100\n";
		povMat<<"scale 1000\n";
		povMat<<"}\n";
		povMat<<"]\n";
		povMat<<"#declare Ind  = Ind + 1; \n";
		povMat<<"#end\n";
		povMat<<"}}\n";
	}
	/* if no map */

    	/*povMat<< finish<<"}"<<endl;*/

    	ostr.str("");

	finish_bright=temp_pattern_bright.str();
	finish_dark=temp_pattern_dark.str();

	/* interior */

    	if ( (matr->mode & MA_RAYMIRROR) || (matr->mode & MA_RAYTRANSP) )
		{

		ostr<<" interior {";

            	ostr<<" ior "<<matr->ang;

            	if (G.scene->r.povmode & R_POV_PHOTONS) ostr<<" caustics 1.0";

            	if (matr->mode & MA_RAYTRANSP)
			{

                	ostr<<" dispersion "<<1.0-matr->YF_dpwr<<" fade_power 1001 ";

                	ostr<<" dispersion_samples ";

                	if (matr->YF_dsmp<2) ostr<<"2";

                	else ostr<<matr->YF_dsmp;

                	const float maxlog = -log(1e-38);

                	float ar = (matr->YF_ar>0) ? -log(matr->YF_ar) : maxlog;

                	float ag = (matr->YF_ag>0) ? -log(matr->YF_ag) : maxlog;

                	float ab = (matr->YF_ab>0) ? -log(matr->YF_ab) : maxlog;

                	float sc = matr->YF_dscale;

                	if (sc!=0.f) sc=1.f/sc;

                	ostr << " fade_color rgb <" << ar*sc << "," << ag*sc << "," << ab*sc << ">";

            		}

            	ostr<<"}";

            	interior=ostr.str();

    		}

	ostr.str("");

	float sr=matr->specr, sg=matr->specg, sb=matr->specb;

	if (matr->spec_shader==MA_SPEC_WARDISO)
		{

		// ........

		sr /= M_PI;

		sg /= M_PI;

		sb /= M_PI;

		}

	/* texture */

	if (!materialneutro)

	for (int m2=0;m2<MAX_MTEX;m2++)
		{

		if (matr->septex & (1<<m2)) continue;// all active channels

		MTex* mtex = matr->mtex[m2];

		if (mtex==NULL) continue;

		Tex* tex = mtex->tex;

		if (tex==NULL) continue;

		map<string, MTex*>::const_iterator mtexL = used_textures.find(string(tex->id.name));
		/* pigment map */

		if (mtexL!=used_textures.end())
			{

               		if (mtex->mapto & MAP_COL)
				{

                    		povMat<<" texture {\n";

				/*blur */
				if ( matr->pov_mat_blur_toggle) {

					povMat<<"#declare BlurAmount="<<matr->pov_mat_blur_amount<<";\n";
					
					povMat<<"average texture_map {\n";
					povMat<<"#declare Ind=0;\n";
					povMat<<"#declare S= seed(0);\n";
				}
                    		povMat<<"pigment {"<< elimChars(mtexL->first)<<"_pigm}";

                    		povMat<<normalGeneral<< finish<<"}"<<endl;

                		}
			/* pattern_map*/


			/*if (mtexL!=used_textures.end())*/
			if ( mtex->mapto & MAP_SPEC)
				{
				if (image_pattern)
					{
				    	in_material_bright<<" texture {";
					/*and here*/

		                	in_material_bright<<"pigment {"<< elimChars(mtexL->first)<<"_pigm}";

		                	in_material_bright<<normalGeneral<< finish_bright<<"}}"<<endl;

				    	in_material_dark<<" texture {";

		                	in_material_dark<<"pigment {"<< elimChars(mtexL->first)<<"_pigm}";

		                	in_material_dark<<normalGeneral<< finish_dark<<"}}"<<endl;
					}
				}

			}

		}

	/* material */

   	povMat<<"#declare "<<elimChars(shader_name)<<" = material { texture {"<<elimChars(shader_name)<<"_tex "<<"} "<<interior<<"}"<<endl;
	/* image_pattern*/
	if (image_pattern)
		{
		povMat<<"// bright + dark"<<endl;
	
		out_pattern_map = in_pattern_map.str();
		out_material_bright = in_material_bright.str();
		out_material_dark = in_material_dark.str();
	
		povMat<<out_material_bright;
		povMat<<out_material_dark;

		povMat<<"// PATTERN_MAP"<<endl;
		povMat<<"#declare "<<elimChars(shader_name)<<" = material { texture { image_pattern {"<<out_pattern_map<<" } ";
		povMat<<"texture_map { [0.0 "<<elimChars(shader_name)<<"_dark ] ";
		povMat<<"[1.0 "<<elimChars(shader_name)<<"_bright ] ";
		povMat<<"}}}"<<endl;
		
		/*clear*/
		in_pattern_map.str("");
		in_material_bright.str("");
		in_material_dark.str("");

		}



	}

/* WRITE ALL OBJECTS	*************************************************************************************************	*/
void PovFileRender_t::writeblur() {
}
/* WRITE ALL OBJECTS	*************************************************************************************************	*/


void PovFileRender_t::writeAllObjects()

{


	for (map<Object*, yafrayObjectRen >::const_iterator obi=all_objects.begin();

			obi!=all_objects.end(); ++obi)

	{

		// skip main duplivert object if in dupliMtx_list, written later

		Object* obj = obi->first;

		if (dupliMtx_list.find(string(obj->id.name))!=dupliMtx_list.end()) continue;

        writeObjectPov(obj, obi->second.obr,obi->second.faces, obj->obmat);

	}



	float obmat[4][4], cmat[4][4], imat[4][4], nmat[4][4];

	for (map<string, vector<float> >::const_iterator dupMtx=dupliMtx_list.begin();

		dupMtx!=dupliMtx_list.end();++dupMtx) {





		for (int i=0;i<4;i++)

			for (int j=0;j<4;j++)

				obmat[i][j] = dupMtx->second[(i<<2)+j];



		MTC_Mat4Invert(imat, obmat);



		Object* obj = dup_srcob[dupMtx->first];

        writeObjectPov(obj, all_objects[obj].obr,all_objects[obj].faces, obmat);



		// all others instances of first

		for (unsigned int curmtx=16;curmtx<dupMtx->second.size();curmtx+=16) {	// number of 4x4 matrices



			// new mtx

			for (int i=0;i<4;i++)

				for (int j=0;j<4;j++)

					nmat[i][j] = dupMtx->second[curmtx+(i<<2)+j];



			MTC_Mat4MulMat4(cmat, imat, nmat);	// transform with respect to original = inverse_original * new



            pov<<"object {"<<elimChars(obj->id.name);

/*            pov<<" matrix <"<<nmat[0][0]<<","<<nmat[0][2]<<","<<nmat[0][1]<<","

                    <<nmat[2][0]<<","<<nmat[2][2]<<","<<nmat[2][1]<<","

                    <<nmat[1][0]<<","<<nmat[1][2]<<","<<nmat[1][1]<<","

                    <<nmat[3][0]<<","<<nmat[3][2]<<","<<nmat[3][1]<<">";*/

            pov<<" matrix <"<<cmat[0][0]<<","<<cmat[0][2]<<","<<cmat[0][1]<<","

                    <<cmat[2][0]<<","<<cmat[2][2]<<","<<cmat[2][1]<<","

                    <<cmat[1][0]<<","<<cmat[1][2]<<","<<cmat[1][1]<<","

                    <<cmat[3][0]<<","<<cmat[3][2]<<","<<cmat[3][1]<<">";



            VlakRen* face0 = all_objects[obj].faces[0];

            Material* face0mat = face0->mat;

            string matname(face0mat->id.name);

            if (matname.length()==0) matname = "blender_default";

            string userLib=getUserMat(matname,PovMat,false);

            string userLibAdd=getUserMat(matname,PovMat,true);

            if (userLib==""){

                    pov<<" material {"<<elimChars(matname)<<"} ";

                    if (userLibAdd!="") pov<<"\n"<<userLibAdd;

                }

                else

                    pov<<"\n"<<userLib;

            if (G.scene->r.povmode & R_POV_PHOTONS)

                    pov<<" photons {target reflection on refraction on} ";

                //pov<<" bounded_by{";

                //boundbox(obj);

                //pov<<" matrix <"<<cmat[0][0]<<","<<cmat[0][2]<<","<<cmat[0][1]<<","

                //    <<cmat[2][0]<<","<<cmat[2][2]<<","<<cmat[2][1]<<","

                //    <<cmat[1][0]<<","<<cmat[1][2]<<","<<cmat[1][1]<<","

                //    <<cmat[3][0]<<","<<cmat[3][2]<<","<<cmat[3][1]<<">";

                //pov<<"}";

            pov<<"}"<<endl;



		}



	}



}

/* WRITE LAMP *********************************************************************************************	*/


void PovFileRender_t::writeAreaLamp(LampRen* lamp, int num, float iview[4][4])

{

	if (lamp->area_shape!=LA_AREA_SQUARE) return;

	float *a=lamp->area[0], *b=lamp->area[1], *c=lamp->area[2], *d=lamp->area[3];

	float power=lamp->energy;



	ostr.str("");

	string md = "off";

	// if no GI used, the GIphotons flag can still be set, so only use when 'full' selected

	if ((re->r.GImethod==2) && (re->r.GIphotons)) { md="on";  power*=re->r.GIpower; }

	// samples not used for GI with photons, can still be exported, is ignored

	int psm=0, sm = lamp->ray_totsamp;

	if (sm>=25) psm = sm/5;

	// transform area lamp coords back to world

	float lpco[4][3];

	MTC_cp3Float(a, lpco[0]);

	MTC_Mat4MulVecfl(iview, lpco[0]);

	MTC_cp3Float(b, lpco[1]);

	MTC_Mat4MulVecfl(iview, lpco[1]);

	MTC_cp3Float(c, lpco[2]);

	MTC_Mat4MulVecfl(iview, lpco[2]);

	MTC_cp3Float(d, lpco[3]);

	MTC_Mat4MulVecfl(iview, lpco[3]);



    if (sm<4) sm=4;

    int smpov=int(sqrt(float(sm)));

    pov<<"light_source {";

    pov<<" <"<<((lpco[0][0]+lpco[1][0]+lpco[2][0]+lpco[3][0])/4.0)<<","<<((lpco[0][2]+lpco[1][2]+lpco[2][2]+lpco[3][2])/4.0)<<","<<((lpco[0][1]+lpco[1][1]+lpco[2][1]+lpco[3][1])/4.0)<<">";

    pov<<" rgb <"<<lamp->r<<","<<lamp->g<<","<<lamp->b<<">";

    pov<<" area_light <"<<(lpco[1][0]-lpco[0][0])<<","<<(lpco[1][2]-lpco[0][2])<<","<<(lpco[1][1]-lpco[0][1])<<">,<"<<(lpco[1][0]-lpco[2][0])<<","<<(lpco[1][2]-lpco[2][2])<<","<<(lpco[1][1]-lpco[2][1])<<">,"<<smpov<<","<<smpov;

    pov<<" adaptive 1";

    if (G.scene->r.povmode & R_POV_PHOTONS)//(ConPhothonesPov==1)

            pov<<" photons {reflection on refraction on}";

    if (G.scene->r.povmode & R_POV_JITTER)//(usaJitterPov==1)

            pov<<" jitter";

     pov<<"}"<<endl;

}
/*milo*/
/*	WRITE SUN	********************************************************************************************************************	*/

void PovFileRender_t::writesun(LampRen* lamp,float iview[4][4])
{
		/* convertblender.c */
		/* render_types.h*/

		float lpco[3], lpvec[3];

		MTC_cp3Float(lamp->co, lpco);
		MTC_Mat4MulVecfl(iview, lpco);
		MTC_cp3Float(lamp->vec, lpvec);
		MTC_Mat4Mul3Vecfl(iview, lpvec);

		/* includes */	
		pov<<"#include \"math.inc\"\n";
		/*1    origin */
		pov<<"#declare x1="<<lpco[0]<<";\n";
		pov<<"#declare y1="<<lpco[2]<<";\n";
		pov<<"#declare z1="<<lpco[1]<<";\n";
		/*2     target */
		pov<<"#declare x2="<<lpco[0] + lpvec[0]<<";\n";
		pov<<"#declare y2="<<lpco[2] + lpvec[2]<<";\n";
		pov<<"#declare z2="<<lpco[1] + lpvec[1]<<";\n";
		/*x3     plan */
		pov<<"#declare x3="<<lpco[0] + lpvec[0]<<";\n"; /*x2*/
		pov<<"#declare z3="<<lpco[1]<<";\n"; /*z1*/
		/*x4     facade */
		pov<<"#declare x4="<<lpco[0] + lpvec[0]<<";\n"; /*x2*/
		pov<<"#declare y4="<<lpco[2]<<";\n"; /*y1*/
		pov<<"#declare z4="<<lpco[1] + lpvec[1]<<";\n";/*z2*/
		/*hdist   horizontal adjacent */
		pov<<"#declare hadj=VDist(<x1,0,z1>,<x3,0,z3>);\n";/* horizontal dist 1-3*/
		/* hhypo  horizontal hypothenuse*/ 
		pov<<"#declare hhypo=VDist(<x1,0,z1>,<x2,0,z2>);\n";/* horizontal dist 1 -2 */
		/*vdist    vertical adjacent */
		pov<<"#declare vadj=VDist(<x1,y1,z1>,<x4,y4,z4>);\n"; /* vertical dist 1-4 */
		/* vhypo   vertical hypothenuse */
		pov<<"#declare vhypo=VDist(<x1,y1,z1>,<x2,y2,z2>);\n"; /* vertical dist 1-2 */
		/*hangle    horizontal angle = horizontal adjacent / hypothenus */
		pov<<"#declare hor=degrees(acos(hadj/hhypo));\n";
		/*vangle    vertical angle = vertical adjacent / hypothenus */
		pov<<"#declare ver=degrees(acos(vadj/vhypo));\n";
		/*Azimuth*/
		/*  
		 *      180
		 *   90     -90
		 *       0
		 */
		pov<<"#if(x2>x1)\n";
		pov<<     "#if(z2>z1)\n";
		pov<<          "#declare Az = -hor-90;\n";
		pov<<     "#else #declare Az = -(90-hor);\n";
		pov<<     "#end\n";
		pov<<"#end\n";

		pov<<"#if(x2<x1)\n";
		pov<<     "#if(z2>z1)\n";
		pov<<          "#declare Az = 90 + hor;\n";
		pov<<     "#else #declare Az = 90- hor;\n";
		pov<<     "#end\n";
		pov<<"#end\n";
		/*Altitude*/
		pov<<"#declare Al = ver;\n";
		/*Bebug*/
		pov<<"#debug \"***************************** SUN  ***************************************\"\n";
		pov<<"#debug concat(\"Azimuth Az:\",str(Az,0,5),\"\\n\")\n";
		pov<<"#debug concat(\"Altitude Al:\",str(Al,0,5),\"\\n\")\n";
		/*Parameters*/
		pov<<"#declare DomeSize= "<<lamp->povsun_size<<"e5;\n";
		pov<<"#declare Current_Turbidity= "<<lamp->povsun_turbidity<<";\n";
		pov<<"#declare Intensity_Mult = "<<lamp->povsun_mult<<";\n";
		pov<<"#declare Tesselation_Trigger= "<<lamp->povsun_tesselation<<";\n";
		pov<<"#declare Max_Vertices= "<<lamp->povsun_max<<";\n";
		pov<<"#declare Min_Vertices= "<<lamp->povsun_min<<";\n";
		pov<<"#declare Horizon_Epsilon= "<<lamp->povsun_hor<<"e-3;\n";
		pov<<"#declare Int_Sun_weight= "<<lamp->povsun_sun_weight<<";\n";
		pov<<"#declare Int_Zenith_weight= "<<lamp->povsun_zenith_weight<<";\n";
		pov<<"#declare Int_Horizon_weight=" <<lamp->povsun_horizon_weight<<";\n";
		char *correction_mode[6];
		if (lamp->povsun_corr==0) *correction_mode = "true " ; else *correction_mode = "false";
		pov<<"#declare UnderHorizonCorrection= "<<*correction_mode<<";\n";
		pov<<"#declare North = z;\n";
		pov<<"#include \"sun.inc\"\n";
		pov<<"light_source{ 0 Light_Color(SunColor,1) translate SolarPosition }\n";

}
/*	WRITE LAMPS	**************************************************************************************************************	*/

void PovFileRender_t::writeLamps()

{

	GroupObject *go;

	int i=0;

	float iview[4][4];

	MTC_Mat4Invert(iview, re->viewmat);



	// all lamps

	for(go=(GroupObject *)re->lights.first; go; go= go->next, i++) {

		LampRen* lamp = (LampRen *)go->lampren;
		/*milo*/
		if (lamp->povsun){ writesun(lamp,iview); return;}

		if (lamp->type==LA_AREA) { writeAreaLamp(lamp,i, iview);  continue; }

		bool is_softL=false, is_sphereL=false;

		if (lamp->type==LA_LOCAL) {

			if (lamp->mode & LA_YF_SOFT) {

				is_softL = true;

			}

			else if ((lamp->mode & LA_SHAD_RAY) && (lamp->YF_ltradius>0.0)) {

				is_sphereL = true;

			}

		}

		// color already premultiplied by energy, so only need distance here

		float pwr = 1;	// default for sun/hemi, distance irrelevant

		if ((lamp->type!=LA_SUN) && (lamp->type!=LA_HEMI)) {

			if (lamp->mode & LA_SPHERE) {

				pwr = lamp->dist*(lamp->dist+1)*(0.25/M_PI);

			}

			else {

				pwr = lamp->dist;

			}

		}



		if (is_sphereL) {

			string md = "off";

			if ((re->r.GImethod==2) && (re->r.GIphotons)) { md="on";  pwr*=re->r.GIpower; }

		}

		string lpmode;

		if ((!is_softL) && (!is_sphereL) && (lamp->type!=LA_YF_PHOTON)) {

			lpmode="off";

			if (lamp->type!=LA_HEMI) {

				if (re->r.mode & R_SHADOW)

					if (((lamp->type==LA_SPOT) //&& (lamp->mode & LA_SHAD)

					) || (lamp->mode & LA_SHAD_RAY)) lpmode="on";

			}

		}

		float ld=0.0;

		//bool has_halo = ((lamp->type==LA_SPOT) && (lamp->mode & LA_HALO) && (lamp->haint>0.0));

		if (lamp->type==LA_SPOT) {

			ld = 1-lamp->spotsi;	//convert back to blender slider setting

			if (ld!=0) ld = 1.f/ld;

		}



		float lpco[3], lpvec[3];

		MTC_cp3Float(lamp->co, lpco);

		MTC_Mat4MulVecfl(iview, lpco);

		MTC_cp3Float(lamp->vec, lpvec);

		MTC_Mat4Mul3Vecfl(iview, lpvec);


	/*light_source */
	    pov<<"light_source {";

	    pov<<" <"<<lpco[0]<<","<<lpco[2]<<","<<lpco[1]<<">";

	    pov<<" rgb <"<<lamp->r<<","<<lamp->g<<","<<lamp->b<<">";

	    /* SPOT */
	    if (lamp->type==LA_SPOT){
		    /* point_at */
		    pov<<" spotlight point_at <"<< lpco[0] + lpvec[0] <<","<<lpco[2] + lpvec[2]<<","<<lpco[1] + lpvec[1]<<">";

		    float fall=acos(lamp->spotsi)*180.0/M_PI;

		    pov<<" falloff "<<fall;
		    pov<<" radius "<<fall-(fall* lamp->spotbl*ld);
	    }

	    if (lamp->type==LA_SUN)

                pov<<" parallel point_at <"<< lpco[0] + lpvec[0] <<","<<lpco[2] + lpvec[2]<<","<<lpco[1] + lpvec[1]<<">";

        else

                pov<<" fade_distance "<<pwr;

	if ( lamp->pov_light_power_fading ) pov <<" fade_power " << lamp->pov_light_power_fading; 
	/* projected_through */
	if (lamp->pov_light_projected_toggle){
		
		pov<<" projected_through { OB"<<lamp->pov.name<<" }";
	}
	    if (lpmode=="off")

                pov<<" shadowless";

        if (G.scene->r.povmode & R_POV_PHOTONS)

                pov<<" photons {reflection on refraction on}";

        pov<<"}"<<endl;

	}

}



/*	WRITE CAMERA ***************************************************************************************************	*/


void PovFileRender_t::writeCamera()

{

    RenderResult *rr= re->result;



    //if (materialneutro)

    //    povIni << "+W" << re->r.xsch << "\n+H" << re->r.ysch<< endl;

    //else

    povIni << "+W" << rr->rectx << "\n+H" << rr->recty<< endl;

	float f_aspect = 1;

	if ((re->r.xsch*re->r.xasp)<=(re->r.ysch*re->r.yasp)) 
	f_aspect = float(re->r.xsch*re->r.xasp)/float(re->r.ysch*re->r.yasp);



	float fdist = 1;	// only changes for ortho

	Camera* cam=NULL;

	if (maincam_obj->type==OB_CAMERA) {

		cam = (Camera*)maincam_obj->data;
		

		if (cam->type & CAM_ORTHO) fdist = cam->ortho_scale*(mainCamLens/32.f);

		string st = "on";

		if (cam->flag & CAM_YF_NO_QMC) st = "off";

	}



			cam = (Camera*)maincam_obj->data;
			/*float shifty = cam->shifty;*/


            pov<<"camera {";

            TextLine *line=(TextLine *)PovCam->lines.first;

            if ((PovCam->nlines==1) && (line->len==0)){

                if (cam->type & CAM_ORTHO)

                    pov<<" orthographic";

                else{
				/*milo*/
					if ( re->r.povshift){
						pov<<"shiftx "<<cam->shiftx<<" shifty "<<cam->shifty<<" ";
						}

                    pov<<" perspective";

                    pov<<" angle "<<2.0 * (atan(1/(2.0*mainCamLens/(f_aspect*32.f))))*(180/M_PI);

                }

                //pov<<" angle "<<atan(0.032/(mainCamLens*0.002))*2.0*(180/M_PI);

            }


            else{

                while (line){

                    pov<<line->line<<endl;

                    line=line->next;

                }

            }

            pov<<" location <"<<maincam_obj->obmat[3][0]<<","<<maincam_obj->obmat[3][2]<<","<<maincam_obj->obmat[3][1]<<">";

            if (cam->type & CAM_ORTHO){

                pov<<" right "<<cam->ortho_scale<<"*x ";

                pov<<" up "<<cam->ortho_scale/(float(re->r.xsch)/float(re->r.ysch))<<"*y ";

            } else

                pov<<" right "<<float(re->r.xsch)/float(re->r.ysch)<<"*x";
				//pov<<" up y*"<<1+shifty/2;

            pov<<" sky <"<<re->viewmat[0][1]<<","<<re->viewmat[2][1]<<","<<re->viewmat[1][1]<<">";

            if (cam->YF_aperture!=0.0){

                pov<<" focal_point <"<<maincam_obj->obmat[3][0] - cam->YF_dofdist*re->viewmat[0][2]<<","<<maincam_obj->obmat[3][2] - cam->YF_dofdist*re->viewmat[2][2]<<","<<maincam_obj->obmat[3][1] - cam->YF_dofdist*re->viewmat[1][2]<<">";

                pov<<" aperture "<<cam->YF_aperture;

                pov<<" blur_samples ";

                if (re->r.YF_AAsamples!=0)

                    pov<<re->r.YF_AAsamples;

                else

                    pov<<"15";

            }

            pov<<" look_at <"<<maincam_obj->obmat[3][0] - fdist * re->viewmat[0][2]<<","<<maincam_obj->obmat[3][2]  - fdist * re->viewmat[2][2]  <<","<<maincam_obj->obmat[3][1] - fdist * re->viewmat[1][2]<<">}"<<endl;

	    //}

}


/*	WRITE WORLD *******************************************************************************************************	*/


bool PovFileRender_t::writeWorld()

{

  World *world = G.scene->world;

  float bg_mult = (re->r.GImethod==0) ? 1 : re->r.GIpower;

  if (world==NULL) return false;

  pov<<"background { color rgb <"<<(world->horr * bg_mult)<<","<<(world->horg * bg_mult)<<","<<(world->horb * bg_mult)<<">}"<<endl;

  return true;

}

/*	EXECUTE YAFRAY	***********************************************************************************************	*/


bool PovFileRender_t::executeYafray(const string &xmlpath)

{

	/*milo*/
	/* ini */
	std::string s;
	std::stringstream out;
	out << re->r.povINI;
	s = out.str();

	char yfr[8];

	string command;

#ifdef WIN32

        if (useMegaPov)

            command = command_path + "mpengine  /render " + "\"" + xmlpath + "Exp.ini\""+" /Exit";

        else

            command = command_path + "pvengine  /render " + "\"" + xmlpath + "Exp.ini\""+" /Exit";

#else



        if (useMegaPov)
		{
			if (re->r.povbeta)
			{
		            command = command_path + "betamegapov  " + "\"" + xmlpath + "Exp" + s + ".ini\"";
			}
			else 
			{

            command = command_path + "mpovray  " + "\"" + xmlpath + "Exp.ini\"";
			}
		}
		else if ( re->r.povbeta)
			command = command_path + "povbeta  " + "\"" + xmlpath + "Exp.ini\"";

        else

            command = command_path + "povray  " + "\"" + xmlpath + "Exp.ini\"";

#endif



#ifndef WIN32

	sigset_t yaf,old;

	sigemptyset(&yaf);

	sigaddset(&yaf, SIGVTALRM);

	sigprocmask(SIG_BLOCK, &yaf, &old);

	int ret=system(command.c_str());

	sigprocmask(SIG_SETMASK, &old, NULL);


	return true;

#else

	int ret=system(command.c_str());

	return ret==0;

#endif



}



/*	STRUCTS	***********************************************************************************************************	*/


typedef struct normAux{ float x,y,z; float cant;}normAux;

typedef struct uvAux{ float ua,va,ub,vb,uc,vc;}uvAux;

typedef struct fcAux{ int a,b,c; string mat;}fcAux;



normAux normaliza(float x,float y,float z)

{

    normAux ret;

    ret.x=x;

    ret.y=y;

    ret.z=z;

    if (x==0.0||y==0.0||z==0.0)

        ret.z=1.0;

    else

    {

        float norm =(float) sqrt(x * x + y * y + z * z);

        ret.x /= norm;

        ret.y /= norm;

        ret.z /= norm;

    }

    return ret;

}



//#include "DNA_mesh_types.h"

//#include "DNA_curve_types.h"

#include "BKE_mesh.h"

//#include "BKE_Curve.h"


/*	MAT BOUND BOX	***********************************************************************************************************	*/


void PovFileRender_t::MatBoundbox(Object* obj)

{

	int i;

	float *vec = NULL;

	if( !obj->bb ) {

		/* ----  Mesh *me; */

		switch ( obj->type ) {

		case OB_MESH:
         /* ****************************************************** */

			/* -- me = (Mesh*)obj->data;*/
         

			/*vec = (float*) mesh_get_bb(me)->vec;*/
         vec = (float*) mesh_get_bb(obj)->vec;

			break;

		}

	} else {

		vec = ( float * ) obj->bb->vec;

	}

	if (vec){

	    float v0[3],v1[3],v3[3],v4[3],xax[3],yax[3],zax[3];

        float tmpvec[4];

        float *from;

        for( i = 0, from = vec; i < 5; i++, from += 3 ){

            memcpy( tmpvec, from, 3 * sizeof( float ) );

            tmpvec[3] = 1.0f;

            Mat4MulVec4fl( obj->obmat, tmpvec );

            tmpvec[0] /= tmpvec[3];

            tmpvec[1] /= tmpvec[3];

            tmpvec[2] /= tmpvec[3];

            switch (i){

                case 0:

                    v0[0]=tmpvec[0];

                    v0[1]=tmpvec[1];

                    v0[2]=tmpvec[2];

                    break;

                case 1:

                    v1[0]=tmpvec[0];

                    v1[1]=tmpvec[1];

                    v1[2]=tmpvec[2];

                    break;

                case 3:

                    v3[0]=tmpvec[0];

                    v3[1]=tmpvec[1];

                    v3[2]=tmpvec[2];

                    break;

                case 4:

                    v4[0]=tmpvec[0];

                    v4[1]=tmpvec[1];

                    v4[2]=tmpvec[2];

                    break;

            }

        }

        xax[0]=v4[0]-v0[0];

        xax[1]=v4[1]-v0[1];

        xax[2]=v4[2]-v0[2];

        yax[0]=v3[0]-v0[0];

        yax[1]=v3[1]-v0[1];

        yax[2]=v3[2]-v0[2];

        zax[0]=v1[0]-v0[0];

        zax[1]=v1[1]-v0[1];

        zax[2]=v1[2]-v0[2];

        povM<<"matrix <"<<xax[0]<<","<<xax[2]<<","<<xax[1]<<","

                            <<zax[0]<<","<<zax[2]<<","<<zax[1]<<","

                            <<yax[0]<<","<<yax[2]<<","<<yax[1]<<","

                            <<v0[0]<<","<<v0[2]<<","<<v0[1]<<">";



	}

}

/* WRITE OBJECT POV ************************************************************************************************************************	*/



void PovFileRender_t::writeObjectPov(Object* obj,ObjectRen *obr, const vector<VlakRen*> &VLR_list1, const float obmat[4][4])

{



    /*float tvec[3];

    float matx[4][4], imatx[4][4];

	MTC_Mat4MulMat4(matx, obj->obmat, re->viewmat);

	MTC_Mat4Invert(imatx, matx);

    VertRen* ver;

	ver = VLR_list1[2]->v1;

	MTC_cp3Float(ver->co, tvec);

	MTC_Mat4MulVecfl(imatx, tvec);

    printf("%f ",tvec[2]);

    */

    if (G.scene->r.povmode & R_POV_NOT_SEND_MESH){

        VlakRen* face0 = VLR_list1[0];

        Material* face0mat = face0->mat;

        string matname(face0mat->id.name);

        if (face0mat->mode & MA_FACETEXTURE) {
            MTFace* tface = RE_vlakren_get_tface(obr, face0, obr->actmtface, NULL, 0);
            if (tface) {

                Image* fimg = (Image*)tface->tpage;

                if (fimg) matname = imgtex_shader[string(face0mat->id.name) + string(fimg->id.name)];

            }

        }

        if (matname.length()==0) matname = "blender_default";

        pov<<"object {"<<elimChars(obj->id.name);

        pov<<" matrix <"<<obmat[0][0]<<","<<obmat[0][2]<<","<<obmat[0][1]<<","

            <<obmat[2][0]<<","<<obmat[2][2]<<","<<obmat[2][1]<<","

            <<obmat[1][0]<<","<<obmat[1][2]<<","<<obmat[1][1]<<","

            <<obmat[3][0]<<","<<obmat[3][2]<<","<<obmat[3][1]<<">";

        string userLib=getUserMat(matname,PovMat,false);

        string userLibAdd=getUserMat(matname,PovMat,true);

        bool PovHF=false;

        if (matname.length()>=7)

            if ((matname[2]=='P') && (matname[3]=='o') && (matname[4]=='v') && (matname[5]=='H') && (matname[6]=='F'))

                PovHF=true;

        if (userLib==""){

            pov<<" material {"<<elimChars(matname)<<"} ";

            if (face0mat->translucency!=0.0) pov<<"double_illuminate ";

            if (userLibAdd!="") pov<<"\n"<<userLibAdd;

        }

        else

            pov<<"\n"<<userLib;

        if (G.scene->r.povmode & R_POV_PHOTONS)

            pov<<" photons {target reflection on refraction on} ";



        if (face0mat->mode & MA_SHLESS)

            pov<<" no_shadow ";



        if (materialneutro){

            if (strcmp(re->r.povAislatedObject,obj->id.name)!=0)

                pov<<" no_image ";

            else

                if (!(face0mat->mode & MA_SHLESS))

                    pov<<" no_shadow ";

        }







        pov<<"}"<<endl;

        return;

    }

    //ordena por smoot

    vector<VlakRen*> solidf;

    vector<VlakRen*> VLR_list;

    int smoots=0;

    for (vector<VlakRen*>::const_iterator fci2=VLR_list1.begin();

				fci2!=VLR_list1.end();++fci2)

	{

		VlakRen* vlr = *fci2;

		if (vlr->flag & R_SMOOTH){

		    smoots++;

		    VLR_list.push_back(vlr);

		    if (vlr->v4) smoots++;

		}

        else{

            solidf.push_back(vlr);

        }

	}

	VLR_list.insert(VLR_list.end(),solidf.begin(),solidf.end());



	VlakRen* face0 = VLR_list[0];

	Material* face0mat = face0->mat;

	string matname(face0mat->id.name);

	// use name in imgtex_shader list if 'TexFace' enabled for this material

	if (face0mat->mode & MA_FACETEXTURE) {
      /*removedObjectRen *obr = face0->obr;*/
      MTFace* tface = RE_vlakren_get_tface(obr, face0, obr->actmtface, NULL, 0);

		/*MTFace* tface = face0->tface;*/

		if (tface) {

			Image* fimg = (Image*)tface->tpage;

			if (fimg) matname = imgtex_shader[string(face0mat->id.name) + string(fimg->id.name)];

		}

	}

	if (matname.length()==0) matname = "blender_default";



    pov<<"object {"<<elimChars(obj->id.name);



    pov<<" matrix <"<<obmat[0][0]<<","<<obmat[0][2]<<","<<obmat[0][1]<<","

        <<obmat[2][0]<<","<<obmat[2][2]<<","<<obmat[2][1]<<","

        <<obmat[1][0]<<","<<obmat[1][2]<<","<<obmat[1][1]<<","

        <<obmat[3][0]<<","<<obmat[3][2]<<","<<obmat[3][1]<<">";



    string userLib=getUserMat(matname,PovMat,false);

    string userLibAdd=getUserMat(matname,PovMat,true);



    bool PovHF=false;

    if (matname.length()>=7)

        if ((matname[2]=='P') && (matname[3]=='o') && (matname[4]=='v') && (matname[5]=='H') && (matname[6]=='F'))

            PovHF=true;



    if (userLib==""){

        pov<<" material {"<<elimChars(matname)<<"} ";

        if (face0mat->translucency!=0.0) pov<<"double_illuminate ";

        if (userLibAdd!="") pov<<"\n"<<userLibAdd;

    }

    else

        pov<<"\n"<<userLib;

    if (G.scene->r.povmode & R_POV_PHOTONS)

        pov<<" photons {target reflection on refraction on} ";



    //pov<<" bounded_by{";

    //boundbox(obj);

    //pov<<"}";



   if (face0mat->mode & MA_SHLESS)

        pov<<" no_shadow ";

   //printf("%s\n",re->r.povAislatedObject);

   //printf("%s\n",obj->id.name);

   if (materialneutro)

        if (strcmp(re->r.povAislatedObject,obj->id.name)!=0)

            pov<<" no_image ";



	//
	//
	// 


	if (obj->povViz)
		pov<<" no_image ";

	if (obj->povShad)
		pov<<" no_shadow ";
	if (obj->povMir)
		pov<<" no_reflection ";

    pov<<"}\n";
	



	bool EXPORT_ORCO = (((face0mat->texco & TEXCO_ORCO)!=0) && (face0->v1->orco!=NULL));

    string has_orco = "off";

	if (EXPORT_ORCO) has_orco = "on";

	bool no_auto = true;	//in case non-mesh, or mesh has no autosmooth

	if (obj->type==OB_MESH) {

		Mesh* mesh = (Mesh*)obj->data;

		if (mesh->flag & ME_AUTOSMOOTH) {

			no_auto = false;

		}

	}

	map<VertRen*, int> vert_idx;	// for removing duplicate verts and creating an index list

	map<int, VertRen*> povVert;

	int uvx=0;

	map<string,int> matlst;

	map<int,string> matlst1;

    int matindex=0;

    int fcindex=0;

	int vidx = 0;	// vertex index counter

	float mat[4][4], imat[4][4];

	MTC_Mat4MulMat4(mat, obj->obmat, re->viewmat);

	MTC_Mat4Invert(imat, mat);



	for (vector<VlakRen*>::const_iterator fci=VLR_list.begin();

				fci!=VLR_list.end();++fci)

	{

		VlakRen* vlr = *fci;

		VertRen* ver;

		//float* orco;

		float tvec[3];

		if (vert_idx.find(vlr->v1)==vert_idx.end()) {

			vert_idx[vlr->v1] = vidx++;

			povVert[vidx-1]=vlr->v1;

			ver = vlr->v1;

			MTC_cp3Float(ver->co, tvec);

			MTC_Mat4MulVecfl(imat, tvec);

			//printf("%f ",tvec[2]);

		}

		if (vert_idx.find(vlr->v2)==vert_idx.end()) {

			vert_idx[vlr->v2] = vidx++;

			povVert[vidx-1]=vlr->v2;

			ver = vlr->v2;

			//MTC_cp3Float(ver->co, tvec);

			//MTC_Mat4MulVecfl(imat, tvec);

		}

		if (vert_idx.find(vlr->v3)==vert_idx.end()) {

			vert_idx[vlr->v3] = vidx++;

			povVert[vidx-1]=vlr->v3;

			ver = vlr->v3;

			//MTC_cp3Float(ver->co, tvec);

			//MTC_Mat4MulVecfl(imat, tvec);

		}

		if ((vlr->v4) && (vert_idx.find(vlr->v4)==vert_idx.end())) {

			vert_idx[vlr->v4] = vidx++;

			povVert[vidx-1]=vlr->v4;

			ver = vlr->v4;

			//MTC_cp3Float(ver->co, tvec);

			//MTC_Mat4MulVecfl(imat, tvec);

		}

	}



    if (!PovHF){



    povM<<"#declare "<<elimChars(obj->id.name)<<" = mesh2 {\n";

    povM<<" vertex_vectors {"<<vidx;

    map<int,normAux> vnormales;

    map<int,uvAux> uvAuxiliar;

    map<int,fcAux> fcAuxiliar;

    normAux tmppto;

    tmppto.x=0;

    tmppto.y=0;

    tmppto.z=0;

    tmppto.cant=0;



    for (map<int,VertRen*>::const_iterator vect1=povVert.begin();vect1!=povVert.end();++vect1)

    {

        vnormales[vect1->first]=tmppto;

        float tvec[3];

        MTC_cp3Float(vect1->second->co, tvec);

        MTC_Mat4MulVecfl(imat, tvec);



        povM<<",<"<< tvec[0]<<","<< tvec[2]<<","<< tvec[1]<<">";

    }



	for (vector<VlakRen*>::const_iterator fci2=VLR_list.begin();

				fci2!=VLR_list.end();++fci2)

	{

		VlakRen* vlr = *fci2;

		Material* fmat = vlr->mat;

		short texco= fmat->texco;

		//bool EXPORT_VCOL = ((fmat->mode & (MA_VERTEXCOL|MA_VERTEXCOLP))!=0);

		string fmatname(fmat->id.name);

		if (fmat->mode & MA_FACETEXTURE) {
         /*removedObjectRen *obr = face0->obr;*/
         MTFace* tface = RE_vlakren_get_tface(obr, face0, obr->actmtface, NULL, 0);

			/*MTFace* tface = vlr->tface;*/

			if (tface) {

				Image* fimg = (Image*)tface->tpage;

				if (fimg) fmatname = imgtex_shader[fmatname + string(fimg->id.name)];

			}

		}

		else if (fmatname.length()==0) fmatname = "blender_default";

		int idx1 = vert_idx.find(vlr->v1)->second;

		int idx2 = vert_idx.find(vlr->v2)->second;

		int idx3 = vert_idx.find(vlr->v3)->second;



		float nvec[3],tvec1[3],tvec2[3],tvec3[3];

		fcAux tmpfc;

        tmpfc.a=idx1;

        tmpfc.b=idx2;

        tmpfc.c=idx3;

        tmpfc.mat=fmatname;

        fcAuxiliar[fcindex++]=tmpfc;

        if (matlst.find(fmatname)==matlst.end())

        {

            matlst[fmatname]=matindex++;

            matlst1[matindex-1]=fmatname;

        }

		// triangle uv and vcol indices

		int ui1=0, ui2=1, ui3=2;

		if (vlr->flag & R_DIVIDE_24) {

			ui3++;

			if (vlr->flag & R_FACE_SPLIT) { ui1++;  ui2++; }

		}

		else if (vlr->flag & R_FACE_SPLIT) { ui2++;  ui3++; }



		uvAux uvtmp;



		/*MTFace* uvc = vlr->tface;	// possible uvcoords (v upside down)*/
      /*removedObjectRen *obr = vlr->obr;*/
      MTFace* uvc = RE_vlakren_get_tface(obr, vlr, obr->actmtface, NULL, 0);

		if (uvc) {

            MTC_cp3Float(vlr->v1->co, tvec1);

            MTC_Mat4MulVecfl(imat, tvec1);

            MTC_cp3Float(vlr->v2->co, tvec2);

            MTC_Mat4MulVecfl(imat, tvec2);

            MTC_cp3Float(vlr->v3->co, tvec3);

            MTC_Mat4MulVecfl(imat, tvec3);

            CalcNormFloat(tvec1,tvec2,tvec3,nvec);

            tmppto=vnormales[idx1];

            tmppto.x+=nvec[0];

            tmppto.y+=nvec[1];

            tmppto.z+=nvec[2];

            tmppto.cant=1.0;

            vnormales[idx1]=tmppto;

            tmppto=vnormales[idx2];

            tmppto.x+=nvec[0];

            tmppto.y+=nvec[1];

            tmppto.z+=nvec[2];

            tmppto.cant=1.0;

            vnormales[idx2]=tmppto;

            //normAux tmppto1;

            tmppto=vnormales[idx3];

            tmppto.x+=nvec[0];

            tmppto.y+=nvec[1];

            tmppto.z+=nvec[2];

            tmppto.cant=1.0;

            vnormales[idx3]=tmppto;



		    uvtmp.ua=uvc->uv[ui1][0];

		    uvtmp.va=uvc->uv[ui1][1]-1;

		    uvtmp.ub=uvc->uv[ui2][0];

		    uvtmp.vb=uvc->uv[ui2][1]-1;

		    uvtmp.uc=uvc->uv[ui3][0];

		    uvtmp.vc=uvc->uv[ui3][1]-1;

		    uvAuxiliar[uvx++]=uvtmp;

		}

		else

		{

		    //cout<<vlr->v1->accum<<","<<vlr->v2->accum<<","<<vlr->v3->accum<<",";

		    if (texco & TEXCO_STRAND) //(vlr->v1->accum+vlr->v2->accum+vlr->v3->accum!=0.0)

		    {

		        MTC_cp3Float(vlr->v1->co, tvec1);

                MTC_Mat4MulVecfl(imat, tvec1);

                MTC_cp3Float(vlr->v2->co, tvec2);

                MTC_Mat4MulVecfl(imat, tvec2);

                MTC_cp3Float(vlr->v3->co, tvec3);

                MTC_Mat4MulVecfl(imat, tvec3);

                CalcNormFloat(tvec1,tvec2,tvec3,nvec);

                tmppto=vnormales[idx1];

                tmppto.x+=nvec[0];

                tmppto.y+=nvec[1];

                tmppto.z+=nvec[2];

                tmppto.cant=1.0;

                vnormales[idx1]=tmppto;

                tmppto=vnormales[idx2];

                tmppto.x+=nvec[0];

                tmppto.y+=nvec[1];

                tmppto.z+=nvec[2];

                tmppto.cant=1.0;

                vnormales[idx2]=tmppto;

                //normAux tmppto1;

                tmppto=vnormales[idx3];

                tmppto.x+=nvec[0];

                tmppto.y+=nvec[1];

                tmppto.z+=nvec[2];

                tmppto.cant=1.0;

                vnormales[idx3]=tmppto;

                //cout << vlr->v1->n[0]<<","<<vlr->v1->n[1]<<","<<vlr->v1->n[2]<<endl;

                /*

		        tmppto.x=vlr->v1->n[0];

		        tmppto.y=vlr->v1->n[1];

		        tmppto.z=vlr->v1->n[2];

		        tmppto.cant=10;

		        vnormales[idx1]=tmppto;

		        tmppto.x=vlr->v2->n[0];

		        tmppto.y=vlr->v2->n[1];

		        tmppto.z=vlr->v2->n[2];

		        tmppto.cant=10;

		        vnormales[idx2]=tmppto;

		        tmppto.x=vlr->v3->n[0];

		        tmppto.y=vlr->v3->n[1];

		        tmppto.z=vlr->v3->n[2];

		        tmppto.cant=10;

		        vnormales[idx3]=tmppto;

                */

		        uvtmp.ua=(vlr->v1->accum+1)/2.0;

		        uvtmp.va=0.0;

                uvtmp.ub=(vlr->v2->accum+1)/2.0;

                uvtmp.vb=0.0;

                uvtmp.uc=(vlr->v3->accum+1)/2.0;;

                uvtmp.vc=0.0;

                uvAuxiliar[uvx++]=uvtmp;

		    }

		    else

		    {

                MTC_cp3Float(vlr->v1->co, tvec1);

                MTC_Mat4MulVecfl(imat, tvec1);

                MTC_cp3Float(vlr->v2->co, tvec2);

                MTC_Mat4MulVecfl(imat, tvec2);

                MTC_cp3Float(vlr->v3->co, tvec3);

                MTC_Mat4MulVecfl(imat, tvec3);

                CalcNormFloat(tvec1,tvec2,tvec3,nvec);

                tmppto=vnormales[idx1];

                tmppto.x+=nvec[0];

                tmppto.y+=nvec[1];

                tmppto.z+=nvec[2];

                tmppto.cant=1.0;

                vnormales[idx1]=tmppto;

                tmppto=vnormales[idx2];

                tmppto.x+=nvec[0];

                tmppto.y+=nvec[1];

                tmppto.z+=nvec[2];

                tmppto.cant=1.0;

                vnormales[idx2]=tmppto;

                //normAux tmppto1;

                tmppto=vnormales[idx3];

                tmppto.x+=nvec[0];

                tmppto.y+=nvec[1];

                tmppto.z+=nvec[2];

                tmppto.cant=1.0;

                vnormales[idx3]=tmppto;

		    }

		}



		if (EXPORT_ORCO) { idx1*=2;  idx2*=2;  idx3*=2; }



		if (vlr->v4) {



			idx1 = vert_idx.find(vlr->v3)->second;

			idx2 = vert_idx.find(vlr->v4)->second;

			idx3 = vert_idx.find(vlr->v1)->second;



            tmpfc.a=idx1;

            tmpfc.b=idx2;

            tmpfc.c=idx3;

            tmpfc.mat=fmatname;

            fcAuxiliar[fcindex++]=tmpfc;



			// increment uv & vcol indices

			ui1 = (ui1+2) & 3;

			ui2 = (ui2+2) & 3;

			ui3 = (ui3+2) & 3;



			if (uvc) {

                tmppto=vnormales[idx2];

                tmppto.x+=nvec[0];

                tmppto.y+=nvec[1];

                tmppto.z+=nvec[2];

                tmppto.cant=1.0;

                vnormales[idx2]=tmppto;



			    uvAux uvtmp;

                uvtmp.ua=uvc->uv[ui1][0];

                uvtmp.va=uvc->uv[ui1][1]-1;

                uvtmp.ub=uvc->uv[ui2][0];

                uvtmp.vb=uvc->uv[ui2][1]-1;

                uvtmp.uc=uvc->uv[ui3][0];

                uvtmp.vc=uvc->uv[ui3][1]-1;

                uvAuxiliar[uvx++]=uvtmp;

			}

            else

            {

                //cout<<vlr->v4->accum<<endl;

                if (texco & TEXCO_STRAND) //(vlr->v1->accum+vlr->v4->accum+vlr->v3->accum!=0.0)

                {

                    /*tmppto.x=vlr->v4->n[0];

                    tmppto.y=vlr->v4->n[1];

                    tmppto.z=vlr->v4->n[2];

                    tmppto.cant=10;

                    vnormales[idx2]=tmppto;

                    */

                    tmppto=vnormales[idx2];

                    tmppto.x+=nvec[0];

                    tmppto.y+=nvec[1];

                    tmppto.z+=nvec[2];

                    tmppto.cant=1.0;

                    vnormales[idx2]=tmppto;



                    uvtmp.ua=(vlr->v1->accum+1)/2.0;

                    uvtmp.va=0.0;

                    uvtmp.ub=(vlr->v4->accum+1)/2.0;

                    uvtmp.vb=0.0;

                    uvtmp.uc=(vlr->v3->accum+1)/2.0;;

                    uvtmp.vc=0.0;

                    uvAuxiliar[uvx++]=uvtmp;

                }

                else

                {

                    tmppto=vnormales[idx2];

                    tmppto.x+=nvec[0];

                    tmppto.y+=nvec[1];

                    tmppto.z+=nvec[2];

                    tmppto.cant=1.0;

                    vnormales[idx2]=tmppto;

                }

            }



			// make sure the indices point to the vertices when orco coords exported

			if (EXPORT_ORCO) { idx1*=2;  idx2*=2;  idx3*=2; }

		}

	}



    povM<<" }\n normal_vectors {"<<vidx;

    for (map<int,normAux>::const_iterator m=vnormales.begin();m!=vnormales.end();++m)

    {

        normAux tmp=m->second;

        if (tmp.cant==1)

            tmp=normaliza(tmp.x,tmp.y,tmp.z);

        povM<<",<"<< tmp.x<<","<< tmp.z<<","<< tmp.y<<">";

    }

    povM<<" }\n";

    if (uvx!=0)

    {

        povM <<" uv_vectors { "<<uvx*3;

        for (map<int,uvAux>::const_iterator m=uvAuxiliar.begin();m!=uvAuxiliar.end();++m)

        {

            uvAux tmp=m->second;

            povM<< ",<"<<tmp.ua << ","<<tmp.va<<">,<"<<tmp.ub<<","<<tmp.vb<<">,<"<<tmp.uc<<","<<tmp.vc<<">";

        }

        povM <<" }\n";

    }

    if (userLib==""){

        povM <<" texture_list { "<<matindex<<",";

        for (map<int,string>::const_iterator m=matlst1.begin();m!=matlst1.end();++m)

        {

            povM <<" texture{"<<elimChars(m->second)<<"_tex"<<"}";

        }

        povM <<"}\n";

    }

    povM <<" face_indices { "<<fcindex;

    for (map<int ,fcAux>::const_iterator m=fcAuxiliar.begin();m!=fcAuxiliar.end();++m)

    {

        fcAux fctmp=m->second;

        int matId = matlst.find(fctmp.mat)->second;

        povM<<",<"<<fctmp.a<<","<<fctmp.b<<","<<fctmp.c<<">";

        if (userLib=="") povM<<","<<matId;

    }

	povM <<"}\n";

	if (smoots==0)

        povM <<" normal_indices {0}\n";

    else {

        povM <<" normal_indices { "<<smoots;

        for (int i=0;i<smoots;i++){

            fcAux fctmp=fcAuxiliar[i];

            povM<<",<"<<fctmp.a<<","<<fctmp.b<<","<<fctmp.c<<">";

        }

        povM <<"}\n";

    }

    if (uvx!=0)

    {

        povM <<" uv_indices { "<<fcindex;

        for(int i=0;i<fcindex;i++)

        {

            int base=i*3;

            povM<<",<"<<base<<","<<base+1<<","<<base+2<<">";

        }

        povM <<"}\n uv_mapping\n";

    }

    povM <<" }\n";



    }

    else{

        if (vidx>4){

            MTex* mtex = face0mat->mtex[0];

            Tex* tex = mtex->tex;

  		    if (tex->type==TEX_IMAGE) {

  		        set<Image*> dupimg;

				Image* ima = tex->ima;

				if (ima) {



				    /*float tvec[4][3];

                    MTC_cp3Float(povVert[0], tvec[0]);

                    MTC_cp3Float(povVert[1], tvec[1]);

                    MTC_cp3Float(povVert[2], tvec[2]);

                    MTC_cp3Float(povVert[4], tvec[3]);

                    for(int i=0;i<4;i++)

                        MTC_Mat4MulVecfl(imat, tvec[i]);

                    float vect[3][3];

                    vect[0][0]=tvec[0][0]-tvec[1][0];

                    vect[0][1]=tvec[0][2]-tvec[1][2];

                    vect[0][2]=tvec[0][1]-tvec[1][1];

                    vect[1][0]=tvec[0][0]-tvec[1][0];

                    vect[1][1]=tvec[0][2]-tvec[1][2];

                    vect[1][2]=tvec[0][1]-tvec[1][1];

                    */



					dupimg.insert(ima);

					string texpath(ima->name);

					adjustPath(texpath);

					string extens="";



					bool encontreExt=false;

					for (int i=texpath.length()-1;i!=0;i--)

                        if (texpath[i]!='.')

                            extens=texpath[i]+extens;

                        else{

                            //extens=strlwr((char *) extens.c_str());

                            extens=strLow(extens);

                            if ((extens=="gif") || (extens=="tiff") || (extens=="tif") || (extens=="bmp") || (extens=="tga") || (extens=="iff") || (extens=="ppm") || (extens=="png") || (extens=="jpeg") || (extens=="jpg") || (extens=="hdr") || (extens=="hdri")){

                                i=1;

                                encontreExt=true;

                            }

                            else

                                extens="";

                        }

                    if (encontreExt){

                        if (extens=="bmp") extens="sys";

                        if (extens=="jpg") extens="jpeg";

                        if (extens=="hdri") extens="hdr";

                        if (extens=="hdr") if (!useMegaPov)  povMat <<"#version unofficial megapov 1.1"<<endl;

                        povM<<"#declare "<<elimChars(obj->id.name)<<" = height_field {";

                        povM<<extens<<" \""<<texpath<<"\" water_level "<<1-mtex->colfac;

                        if (smoots!=0)

                            povM<<" smooth";

                        MatBoundbox(obj);

                        povM<<" }\n";

                        //povM<<" matrix <"<<obmat[0][0]<<","<<obmat[0][2]<<","<<obmat[0][1]<<","

                        //    <<obmat[2][0]<<","<<obmat[2][2]<<","<<obmat[2][1]<<","

                        //    <<obmat[1][0]<<","<<obmat[1][2]<<","<<obmat[1][1]<<","

                        //    <<obmat[3][0]<<","<<obmat[3][2]<<","<<obmat[3][1]<<">}\n";

                    }

                }

  		    }



            /*povVert[]

            for (map<int,VertRen*>::const_iterator vect1=povVert.begin();vect1!=povVert.end();++vect1)

            {

                vnormales[vect1->first]=tmppto;

                float tvec[3];

                MTC_cp3Float(vect1->second->co, tvec);

                MTC_Mat4MulVecfl(imat, tvec);

                povM<<",<"<< tvec[0]<<","<< tvec[2]<<","<< tvec[1]<<">";

            }*/

        }

        else

            cout<<"Objeto "<<obj->id.name<<" no tiene suficientes vertices para Height Field"<<endl;

    }



}

/*	IS TO POV **********************************************************************************************	*/


bool PovFileRender_t::isToPov ()

{

    Text *txt_iter;

    txt_iter = (Text *)G.main->text.first;

    int total=0;

    while( ( txt_iter ) ){

        if( strcmp( "PovMat", txt_iter->id.name+2) == 0 ){

            PovMat=txt_iter;

            total++;

        }

        if( strcmp( "PovCam", txt_iter->id.name+2) == 0 ){

            PovCam=txt_iter;

            total++;

        }

        if( strcmp( "PovGlobalRad", txt_iter->id.name+2) == 0 ){

            PovGlobalRad=txt_iter;

            total++;

        }

        if( strcmp( "PovGlobalPhot", txt_iter->id.name+2) == 0 ){

            PovGlobalPhot=txt_iter;

            total++;

        }

        if( strcmp( "PovInit", txt_iter->id.name+2) == 0 ){

            PovInit=txt_iter;

            total++;

        }

        if( strcmp( "PovCommand-line", txt_iter->id.name+2) == 0 ){

            PovIniCommand=txt_iter;

            total++;

        }

        txt_iter = (Text *)txt_iter->id.next;

    }



    defAmbPov=0.05;

    defAmbPov=G.scene->r.PovDefaultAmbient;

    tipRadPov=G.scene->r.PovRadio;

    if (strlen(re->r.povAislatedObject)==0)

        materialneutro=false;

    else

        materialneutro=true;

    if (total==6)

        return true;

    else

        return false;



}

/* WRITE MODULATORS ****************************************************************************************	*/


void PovFileRender_t::writeMaterialsAndModulators(){

	for (map<string, Material*>::const_iterator blendmat=used_materials.begin();

			blendmat!=used_materials.end();++blendmat)

	{



		Material* matr = blendmat->second;

		/*for (int m=0;m<MAX_MTEX;m++)

		{

			if (matr->septex & (1<<m)) continue;// all active channels

			MTex* mtex = matr->mtex[m];

			if (mtex==NULL) continue;

			Tex* tex = mtex->tex;

			if (tex==NULL) continue;

			map<string, MTex*>::const_iterator mtexL = used_textures.find(string(tex->id.name));

		}*/

		writeShader(blendmat->first, matr);

	}



	// write the mappers & shaders for the TexFace case

	if (!imagetex.empty()) {

		//int snum = 0;

		for (map<Image*, set<Material*> >::const_iterator imgtex=imagetex.begin();

				imgtex!=imagetex.end();++imgtex)

		{



			for (set<Material*>::const_iterator imgmat=imgtex->second.begin();

					imgmat!=imgtex->second.end();++imgmat)

			{

				Material* matr = *imgmat;

				string shader_name = ostr.str();

				imgtex_shader[string(matr->id.name) + string(imgtex->first->id.name)] = shader_name;

				writeShader(shader_name, matr, ostr.str());

			}

		}

	}

}



void PovFileRender_t::writeObject(Object* obj,ObjectRen *obr, const std::vector<VlakRen*> &VLR_list, const float obmat[4][4]){}



void PovFileRender_t::writeHemilight(){}



void PovFileRender_t::writePathlight(){}




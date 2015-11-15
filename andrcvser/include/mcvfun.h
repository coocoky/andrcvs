#ifndef MCVFUN_H
#define MCVFUN_H

#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d.hpp>

#include <string>
#include <fstream>
#include <iostream>
#include <memory>
#include <functional>
#include <queue>
#include <set>
#include <map>
#include <vector>
#include <cstring>

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <iconv.h>

#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
//#include <curses.h>

//#define  MAX_PATH  512
#define  MAX_PATH  PATH_MAX
#define  RPC_PORT  9090
#define  UDP_PORT  9092

#define  VOCA_COLS   4000
#define  SIZE_VOC    400

#define  ZOOM_SIZE      320
#define  IMG_MAX_SIZE   320
#define  MIN_IMG_WH     120
#define  MIN_ZOOM_WH    120
#define  ROI_WH         100
#define  MIN_KEYS       120

#define  FAST_TH      5

#define  MIN_ROIS_KEYS   160

#define  STEP   5
#define  KNN    5

#define  MWH     (IMG_MAX_SIZE/5)

#define  OUTLEN      1024
#define  RADIUS      15.0
#define  EXTRACTOR   DAISY

using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;

//const char char_lang[] = "utf-8";
//const char char_lang[] = "gbk";

struct  LlcData
{
    int     m5_w;
    int     m5_h;
    int     m5_wh;
    uchar   llc_mask[MWH][MWH];
    float   llc_idx[MWH][MWH][KNN];
    float   llc_weight[MWH][MWH][KNN];
};

struct  ZoomRoiData
{
    float      float_hists[MWH][MWH][SIZE_VOC];
    int        mask[MWH][MWH];
    int        num_mask[MWH][MWH];
    int        m5_wh;
};


typedef  vector<Point>  VecPoint;

void  opencv_llc_bow_Descriptor(Mat &image, Mat &vocabulary,  vector<KeyPoint> &key_points, Mat &mat_llc_descriptor );
void  opencv_llc_bow_mats(Mat &image, Mat &vocabulary, Mat &mat_mask, Mat &descriptors, float radius, LlcData *p_llc_data);
void  list_dirs( const string& directory, vector<string>  &entries);
void  list_files(vector<string>  &dir_paths, vector<string>  &file_paths);
void  list_images(vector<string>  &file_paths,  vector<string>  &image_paths);

void  draw_hist(char *win_name, Mat hist);
void  filter_hist(Mat  &hist);
void  class_bow_save(std::string &file_hash, string  &str_path, set<string>  &class_names, LlcData  *p_llc_data, map<string, VecPoint >  &map_class_rois_points);

void  mcv_img_zoom(Mat  &image, Mat  &image_zoom, int  zwh);
bool  img_zoom_hist(Mat  &image,  Mat &vocabulary, Mat &voc_matchs, int  zoom_wh, Mat  &img_zoom, Mat &main_hist, LlcData  *p_llc_data, ZoomRoiData  &zoom_roi);

void  create_imgs_db(string  &sqlite_fn);
bool  read_db_map(string &sqlite_fn, map<string, string> &hash_fn_map);
bool  img_to_db(sqlite3 *db, string  &str_im_hash, string &str_im_path, string &result_path, Mat &main_hist, Mat &mat_mask, LlcData *p_llc_data);

double  compare_class_hists(set<string>  &class_names, map<string, Mat>  &map_class_hists01, map<string, Mat>  &map_class_hists02);

void  read_class_db(string &df_path,  set<string>  &class_names, map<string, Mat>  &map_class_vocs, vector<string>  &strs_rois_class, Mat  &class_hists);

bool  img_zoom_class_hists(Mat &image, Mat &vocabulary, Mat &voc_matchs, int  zoom_wh, set<string>  &class_names,  vector<string>  &strs_rois_class,
                           Mat   &class_hists,  Mat &main_hist,  map<string, Mat>  &map_class_hists);

bool  img_zoom_class_rois(Mat &image, Mat &vocabulary, Mat &voc_matchs, int  zoom_wh, set<string>  &class_names,  vector<string>  &strs_rois_class,
                           Mat   &class_hists,  Mat &main_hist, LlcData  *p_llc_data,  map<string, VecPoint >  &map_class_rois_points);

void  llc_data_to_class_hists(set<string>  &class_names, Mat &voc_matchs, LlcData  *p_llc_data, map<string, VecPoint >  &map_class_rois_points,  map<string, Mat>  &map_class_hists);

/*
class CodeConverter
{
private:
    iconv_t cd;

public:

    CodeConverter(const char *from_charset, const char *to_charset)
    {
        cd = iconv_open(to_charset,from_charset);
    }

    int convert(char *inbuf,int inlen,char *outbuf,int outlen)
    {
        char **pin = &inbuf;
        char **pout = &outbuf;

        memset(outbuf,0,outlen);
        return iconv(cd,pin,(size_t *)&inlen,pout,(size_t *)&outlen);
    }

    void convert_string(const string  &str_in, string &str_out)
    {
        char ch_buf[OUTLEN];

        convert((char*)str_in.c_str(), str_in.length(), ch_buf, OUTLEN);

        str_out = ch_buf;
    }

    ~CodeConverter()
    {
        iconv_close(cd);
    }
};
*/

#endif

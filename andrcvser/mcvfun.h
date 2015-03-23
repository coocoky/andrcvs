#ifndef MCVFUN_H
#define MCVFUN_H

#include <opencv2/opencv.hpp>
#include <opencv2/nonfree/nonfree.hpp>

#include <string>
#include <fstream>
#include <iostream>
#include <memory>
#include <functional>
#include <queue>
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

//#define  MAX_PATH  512
#define  RPC_PORT  9090
#define  UDP_PORT  9092

#define  VOCA_COLS   2000
#define  SIZE_VOC    200

#define  ZOOM_SIZE    240
#define  MIN_IMG_WH   120
#define  MIN_ZOOM_WH  100
#define  MIN_KEYS     120
#define  MIN_KPS      3
#define  FASTFDP      5

#define  STEP   5
#define  KNN    5
#define  MWH    47
#define  SURF_SIZE   10
#define  OUTLEN      1024

using namespace  std;
using namespace  cv;

//const char char_lang[] = "utf-8";
//const char char_lang[] = "gbk";

struct  LlcData
{
    uchar   llc_mask[MWH][MWH];
    float   llc_idx[MWH][MWH][KNN];
    float   llc_weight[MWH][MWH][KNN];
};

bool  voc_bow_mats(Mat  &image,  Mat &vocabulary,  Mat &voc_matchs, Mat &main_hist, Mat &voc_hists, Mat &mat_mask, LlcData *p_llc_data);
void  opencv_llc_bow_Descriptor(Mat &image, Mat &vocabulary,  vector<KeyPoint> &key_points, Mat &mat_llc_descriptor );
void  opencv_llc_bow_mats(Mat &image, Mat &vocabulary, Mat &mat_mask, Mat  &descriptors, LlcData *p_llc_data);
void  list_dirs( const string& directory, vector<string>  &entries);
void  list_files(vector<string>  &dir_paths, vector<string>  &file_paths);
void  list_images(vector<string>  &file_paths,  vector<string>  &image_paths);

void  create_imgs_db(string  &sqlite_fn);
bool  read_db_map(string &sqlite_fn, map<string, string> &hash_fn_map);
bool  img_to_db(sqlite3 *db, string  &str_im_hash, string &str_im_path, string &result_path, Mat &main_hist, Mat &mat_mask, LlcData *p_llc_data);
bool  voc_db_mats(sqlite3 *db, string  &str_im_hash, string  &result_path, Mat &vocabulary,  Mat &voc_matchs, string  &str_im_fn,  Mat &main_hist, Mat &voc_hists);
bool  sort_match_imgs(string  &sqlite_fn, Mat &vocabulary, string &result_path, Mat &voc_matchs, Mat &hists01, std::vector<std::string> &img_match_hashs, multimap<double, string, greater<double> >  &map_hists);
bool  sort_surf120_imgs(Mat  &image, string  &sqlite_fn, string  &imgs_path, std::vector<std::string> &img_match_hashs, multimap<double, string, greater<double> >  &map_hists);

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

#endif

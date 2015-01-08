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

#define  ZOOM_SIZE   240
#define  MIN_IMG_WH  200
#define  MIN_KPS     3

#define  STEP   5
#define  KNN    5

#define  OUTLEN  1024

using namespace  std;
using namespace  cv;

//const char char_lang[] = "utf-8";
//const char char_lang[] = "gbk";

void  make_datas(string path);
Mat   surf_hist_120(Mat  &image, Ptr<DescriptorExtractor>  &extractor, float angle, int  size_surf);
//bool  calc_hist(Mat  &image,  Mat &vocabulary, Ptr<FeatureDetector> detector, Mat &sum_query_descriptor);
bool  calc_hist(Mat  &image,  Mat &vocabulary, Mat &sum_query_descriptor);
bool  voc_hist(Mat  &img_src,  Mat &vocabulary,  Mat &voc_matchs, Mat &voc_hist);
void  opencv_llc_bow_Descriptor(Mat &image, Mat &vocabulary,  vector<KeyPoint> &key_points, Mat &mat_llc_descriptor );

void  list_dirs( const string& directory, vector<string>  &entries);
void  list_files(vector<string>  &dir_paths, vector<string>  &file_paths);
void  list_images(vector<string>  &file_paths,  vector<string>  &image_paths);
void  create_imgs_db(string  &sqlite_fn);

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

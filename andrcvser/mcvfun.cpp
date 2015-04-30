#include "mcvfun.h"
#include <stdio.h>

Mat  surf_hist_120(Mat  &image, Ptr<DescriptorExtractor>  &extractor, float angle, int  size_surf)
{
    vector<KeyPoint> key_points;

    KeyPoint   key_point;

    for (int y=10;  y<=120-10;  y+=20)
    {
        for(int x=10;  x<=120-10; x+=20)
        {

            key_point.pt.x = x;
            key_point.pt.y = y;
            key_point.angle = angle;
            key_point.size = size_surf;

            key_points.push_back(key_point);
        }

    }

    Mat descriptors;

    extractor -> compute( image, key_points, descriptors );

    int  size_hist = key_points.size()* descriptors.cols;

    Mat  hist = Mat::zeros(1, size_hist, CV_32F);

    //std::cout << " size_hist : " << size_hist << std::endl;

    int  idx = 0;

    for (int y=0;  y<descriptors.rows;  y++)
    {
        for (int x=0;  x<descriptors.cols; x++)
        {
            float   ve = descriptors.at<float>(y,x);

            hist.at<float>(0, idx) = ve * 100.0;

            idx++;
        }
    }

    return  hist;
}

bool  voc_db_mats(sqlite3 *db, string  &str_im_hash, string  &result_path, Mat &vocabulary,  Mat &voc_matchs, string  &str_im_fn, Mat &main_hist, Mat &voc_key_hists)
{
    Mat   voc_hist = Mat::zeros(1, SIZE_VOC, CV_32F);
    Mat   sum_query_descriptor = Mat::zeros(1, VOCA_COLS, CV_32F);

    Mat   descriptors;
    Mat   mat_mask;

    sqlite3_stmt  *stmt;
    const   char  *zTail;

    char    ch_cmd[2048];

    sprintf(ch_cmd, "SELECT id, imhash, impath, imw, imh, imhist FROM imgs WHERE imhash='%s';", str_im_hash.c_str());

    int   result = sqlite3_prepare(db, ch_cmd, -1, &stmt, &zTail);

    result = sqlite3_step(stmt);

    if (result != SQLITE_ROW ) return false;

    LlcData  *p_llc_data = new LlcData;
    memset(p_llc_data, 0, sizeof(LlcData));

    BowRoiData   *p_bow_rois = new BowRoiData;

    for (int my=0; my<MWH; my++)
    {
        for (int mx=0; mx<MWH; mx++)
        {
              p_bow_rois->mask[my][mx] = 0;
              p_bow_rois->m[my][mx] = 0;
        }
    }

    int             id = sqlite3_column_int( stmt, 0 );
    const char* imhash = (char*)sqlite3_column_text( stmt, 1 );
    const char* impath = (char*)sqlite3_column_text( stmt, 2 );

    str_im_fn = impath;

    int   imw = sqlite3_column_int( stmt, 3);
    int   imh = sqlite3_column_int( stmt, 4);

    int   mrows = imh/5-1;
    int   mcols = imw/5-1;

    const void   *p_buf = sqlite3_column_blob(stmt, 5);
    int   size_buf = sqlite3_column_bytes(stmt, 5);

    main_hist = Mat::zeros(1, SIZE_VOC, CV_32F);

    vector<float>   hist_data;
    hist_data.resize(main_hist.cols);

    char  *p_data = (char*)(&hist_data[0]);

    memmove(p_data,  p_buf, size_buf);

    for (int i=0; i<main_hist.cols; i++)
    {
        main_hist.at<float>(0, i) = hist_data[i];
    }

    mat_mask = Mat::zeros(mrows, mcols, CV_8UC1);

    string   str_fn_bow = result_path;

    str_fn_bow += "/bow_";
    str_fn_bow += str_im_hash;
    str_fn_bow += ".dat";

    fstream  fs_bow(str_fn_bow.c_str(), ios_base::binary | ios_base::in);
    fs_bow.read((char*)(p_llc_data->llc_mask), sizeof(p_llc_data->llc_mask));
    fs_bow.read((char*)(p_llc_data->llc_idx), sizeof(p_llc_data->llc_idx));
    fs_bow.read((char*)(p_llc_data->llc_weight), sizeof(p_llc_data->llc_weight));
    fs_bow.close();

    for (int r=0; r<mat_mask.rows; r++)
    {
        for (int c=0; c<mat_mask.cols; c++)
        {
            mat_mask.at<uchar>(r, c) = p_llc_data->llc_mask[r][c];
        }
    }

    int  wh=(MIN_ZOOM_WH)/5-1;

    for (int y0=0; y0<=mat_mask.rows-wh; y0+=1)
    {
        for (int x0=0; x0<=mat_mask.cols-wh; x0+=1)
        {
            sum_query_descriptor = Mat::zeros(1, VOCA_COLS, CV_32F);

            int m;
            m = 0;
            for (int y=y0; y<y0+wh; y++)
            {
                for (int x=x0; x<x0+wh; x++)
                {
                    if (p_llc_data->llc_mask[y][x] < 200) continue;

                    for (int n=0; n<5; n++)
                    {
                        int  idx = p_llc_data->llc_idx[y][x][n]+0.5;
                        sum_query_descriptor.at<float>(0, idx) += p_llc_data->llc_weight[y][x][n];
                    }
                    m++;
                }
            }

            sum_query_descriptor = sum_query_descriptor/(1.0*m)*1000.0;

            int  voc_knn = voc_matchs.cols;

            //std::cout << "voc_knn : " << voc_knn << std::endl;

            voc_hist = Mat::zeros(1, SIZE_VOC, CV_32F);

            for (int n=0; n<voc_matchs.rows; n++)
            {
                for (int i=0; i<voc_knn; i++)
                {
                    int  idx = voc_matchs.at<int>(n, i);

                    voc_hist.at<float>(0, n) += sum_query_descriptor.at<float>(0, idx);

                }
            }

            p_bow_rois->mask[y0][x0] = 1;
            p_bow_rois->m[y0][x0] = m;
            p_bow_rois->voc_hists[y0][x0] = voc_hist.clone();
        }
    }

    for (int y=0; y<=mat_mask.rows-wh; y++)
    {
        for (int x=0; x<=mat_mask.cols-wh; x++)
        {
            int  m = p_bow_rois->m[y][x];
            Mat  voc_hist = p_bow_rois->voc_hists[y][x];

            int  m_max = 1;

            for (int my=-1; my<=1; my++)
            {
                for (int mx=-1; mx<=1; mx++)
                {
                    int  y1 = y + my;
                    int  x1 = x + mx;

                    int  m1 = p_bow_rois->m[y1][x1];

                    if (y1<0 || y1>mat_mask.rows-wh || x1<0 || x1>mat_mask.cols-wh) continue;
                    if (p_bow_rois->mask[y1][x1] == 0) continue;

                    if (m < m1) { m_max = 0; break; }
                }
            }

            if (m > MIN_ROIS_KEYS && m_max == 1 ) voc_key_hists.push_back(voc_hist);
        }
    }

    delete   p_bow_rois;
    delete   p_llc_data;

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return true;
}

bool  img_to_db(sqlite3 *db, string  &str_im_hash, string &str_im_path, string &result_path, Mat &main_hist, Mat &mat_mask, LlcData *p_llc_data)
{
    char *errmsg = 0;
    int   ret = 0;

    sqlite3_stmt  *stmt;
    const   char  *zTail;

    sqlite3_prepare_v2(db, "insert into imgs(id, imhash, impath, imw, imh, imhist) values(null,?,?,?,?,?)", -1, &stmt, 0);

    sqlite3_bind_text(stmt, 1, str_im_hash.c_str(), str_im_hash.length(), NULL);
    sqlite3_bind_text(stmt, 2, str_im_path.c_str(), str_im_path.length(), NULL);
    sqlite3_bind_int(stmt, 3, (mat_mask.cols+1)*5);
    sqlite3_bind_int(stmt, 4, (mat_mask.rows+1)*5);

    vector<float>  hist_data;

    hist_data.clear();
    hist_data.resize(main_hist.cols);

    for (int i=0; i<main_hist.cols; i++)
    {
        hist_data[i] = main_hist.at<float>(0, i);
    }

    char   *p_buf = (char*)(&hist_data[0]);
    sqlite3_bind_blob(stmt, 5, p_buf, main_hist.cols*sizeof(float), NULL);

    string   str_fn_bow = result_path;

    str_fn_bow += "/bow_";
    str_fn_bow += str_im_hash;
    str_fn_bow += ".dat";

    fstream  fs_bow(str_fn_bow.c_str(), ios_base::binary | ios_base::out);
    fs_bow.write((char*)(p_llc_data->llc_mask), sizeof(p_llc_data->llc_mask));
    fs_bow.write((char*)(p_llc_data->llc_idx), sizeof(p_llc_data->llc_idx));
    fs_bow.write((char*)(p_llc_data->llc_weight), sizeof(p_llc_data->llc_weight));
    fs_bow.close();

    int  ret_db = sqlite3_step(stmt);

    sqlite3_reset(stmt);
    //sqlite3_finalize(stmt);
    //sqlite3_free(errmsg);
}

bool  voc_bow_mats(Mat  &image,  Mat &vocabulary,  Mat &voc_matchs, Mat &main_hist, Mat &voc_key_hists, Mat &mat_mask, int ms, LlcData *p_llc_data)
{
    FastFeatureDetector  fast_detector(FASTFDP);

    main_hist = Mat::zeros(1, SIZE_VOC, CV_32F);

    memset(p_llc_data, 0, sizeof(LlcData));

    int    width01;
    int    high01;

    int    *p_l = &width01;
    int    *p_s = &high01;

    int    img_ls = image.cols;
    int    img_ss = image.rows;

    if (image.cols < image.rows)
    {
        p_s =  &width01;
        p_l =  &high01;
        img_ls = image.rows;
        img_ss = image.cols;
    }

    *p_l = ZOOM_SIZE;
    float  zoom = *p_l*1.0/img_ls;
    *p_s = img_ss*zoom + 0.5;

    Mat    image_zoom = Mat::zeros( high01, width01, CV_8UC1);
    Mat    image_draw = Mat::zeros( high01, width01, CV_8UC1);

    resize(image, image_zoom, Size(width01, high01));

    if (high01 < MIN_ZOOM_WH || width01 < MIN_ZOOM_WH) return false;

    vector<KeyPoint>  key_points;

    fast_detector.detect( image_zoom, key_points);

    for (int i=0; i<key_points.size(); i++)
    {
        Point  point(key_points[i].pt.x, key_points[i].pt.y);

        circle( image_draw, point, STEP, Scalar( 255, 255, 255 ), -1);
    }

    mat_mask = Mat::zeros(image_zoom.rows/5-1, image_zoom.cols/5-1, CV_8UC1);

    int  n_mask=0;

    for (int my=0; my<mat_mask.rows; my++)
    {
        for (int mx=0; mx<mat_mask.cols; mx++)
        {
            int x = mx*5+5;
            int y = my*5+5;

            if (image_draw.at<uchar>(y, x) > 200 )
            {
                mat_mask.at<uchar>(my, mx) = 255;
                n_mask++;
            }
        }
    }

    if (n_mask < MIN_KEYS) return false;

    BowRoiData   *p_bow_rois = new BowRoiData;

    for (int my=0; my<MWH; my++)
    {
        for (int mx=0; mx<MWH; mx++)
        {
              p_bow_rois->mask[my][mx] = 0;
              p_bow_rois->m[my][mx] = 0;
        }
    }


    Mat   descriptors;

    opencv_llc_bow_mats(image_zoom, vocabulary, mat_mask, descriptors, SURF_SIZE+ms, p_llc_data);

    Mat  sum_query_descriptor = Mat::zeros(1, VOCA_COLS, CV_32F);

    int m=0;
    for (int y=0; y<mat_mask.rows; y++)
    {
        for (int x=0; x<mat_mask.cols; x++)
        {
            if (p_llc_data->llc_mask[y][x] < 200) continue;

            for (int n=0; n<5; n++)
            {
                int idx = p_llc_data->llc_idx[y][x][n]+0.5;
                sum_query_descriptor.at<float>(0, idx) += p_llc_data->llc_weight[y][x][n];
            }
            m++;
        }
    }

    sum_query_descriptor = sum_query_descriptor/(1.0*m)*1000.0;

    int  voc_knn = voc_matchs.cols;

    //std::cout << "voc_knn : " << voc_knn << std::endl;

    for (int n=0; n<voc_matchs.rows; n++)
    {
        for (int i=0; i<voc_knn; i++)
        {
            int  idx = voc_matchs.at<int>(n, i);

            main_hist.at<float>(0, n) += sum_query_descriptor.at<float>(0, idx);
        }
    }

    int  wh=(MIN_ZOOM_WH+ms*5)/5-1;

    for (int y0=0; y0<=mat_mask.rows-wh; y0+=1)
    {
        for (int x0=0; x0<=mat_mask.cols-wh; x0+=1)
        {
            sum_query_descriptor = Mat::zeros(1, VOCA_COLS, CV_32F);

            m = 0;
            for (int y=y0; y<y0+wh; y++)
            {
                for (int x=x0; x<x0+wh; x++)
                {
                    if (p_llc_data->llc_mask[y][x] < 200) continue;

                    for (int n=0; n<5; n++)
                    {
                        int idx = p_llc_data->llc_idx[y][x][n]+0.5;
                        sum_query_descriptor.at<float>(0, idx) += p_llc_data->llc_weight[y][x][n];
                    }

                    m++;
                }
            }

            sum_query_descriptor = sum_query_descriptor/(1.0*m)*1000.0;

            int  voc_knn = voc_matchs.cols;

            //std::cout << "voc_knn : " << voc_knn << std::endl;

            Mat  voc_hist = Mat::zeros(1, SIZE_VOC, CV_32F);

            for (int n=0; n<voc_matchs.rows; n++)
            {
                for (int i=0; i<voc_knn; i++)
                {
                    int  idx = voc_matchs.at<int>(n, i);

                    voc_hist.at<float>(0, n) += sum_query_descriptor.at<float>(0, idx);

                }
            }

            p_bow_rois->mask[y0][x0] = 1;
            p_bow_rois->m[y0][x0] = m;
            p_bow_rois->voc_hists[y0][x0] = voc_hist.clone();
        }
    }

    for (int y=0; y<=mat_mask.rows-wh; y++)
    {
        for (int x=0; x<=mat_mask.cols-wh; x++)
        {
            int  m = p_bow_rois->m[y][x];
            Mat  voc_hist = p_bow_rois->voc_hists[y][x];

            int  m_max = 1;

            for (int my=-1; my<=1; my++)
            {
                for (int mx=-1; mx<=1; mx++)
                {
                    int  y1 = y + my;
                    int  x1 = x + mx;

                    int  m1 = p_bow_rois->m[y1][x1];

                    if (y1<0 || y1>mat_mask.rows-wh || x1<0 || x1>mat_mask.cols-wh) continue;
                    if (p_bow_rois->mask[y1][x1] == 0) continue;

                    if (m < m1) { m_max = 0; break; }
                }
            }

            if (m > MIN_ROIS_KEYS && m_max == 1 ) voc_key_hists.push_back(voc_hist);
        }
    }

    delete  p_bow_rois;

    return  true;
}

void  opencv_llc_bow_mats(Mat &image, Mat &vocabulary, Mat &mat_mask, Mat  &descriptors, int surf_size, LlcData *p_llc_data)
{
    //std::cout << "opencv_llc_bow_Descriptor" << std::endl;

    Ptr<DescriptorExtractor> extractor = DescriptorExtractor::create( "SURF" );

    vector<KeyPoint> key_points;
    vector<KeyPoint> key_points01;

    KeyPoint   key_point;

    for (int y=0; y<mat_mask.rows; y++)
    {
        for (int x=0; x<mat_mask.cols; x++)
        {
            key_point.pt.x = 5*x;
            key_point.pt.y = 5*y;
            key_point.angle = 0.0;
            key_point.size = surf_size;

            key_points.push_back(key_point);

            KeyPoint   key_point01 = key_point;

            key_point01.pt.x += 5;
            key_point01.pt.y += 5;

            key_points01.push_back(key_point01);
        }
    }


    extractor->compute( image, key_points01, descriptors );

    int     knn = 5;
    float   fbeta = 1e-4;

    vector<vector<DMatch> > matches;

    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce" );
    //Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "FlannBased" );

    matcher->knnMatch( descriptors, vocabulary, matches, knn );

    for (int icx=0; icx<descriptors.rows; icx++)
    {
        vector<DMatch> &matchesv = matches[icx];

        Mat   des_mat_r1 = descriptors.row(icx);

        Mat   mat_cbknn;
        mat_cbknn.release();

        for (int i=0; i<knn; i++)
        {
            Mat  mat_idx = vocabulary.row(matchesv[i].trainIdx);

            mat_cbknn.push_back(mat_idx);
        }

        Mat  ll_mat = Mat::eye(knn, knn, CV_32FC1);
        Mat  z_mat = mat_cbknn - repeat(des_mat_r1, 5, 1);
        Mat  one_mat = Mat::ones(knn, 1, CV_32FC1);
        Mat  c_mat = z_mat*z_mat.t();

        float  ftrace = trace(c_mat).val[0];

        c_mat = c_mat + ll_mat*fbeta*ftrace;

        Mat  w_mat = c_mat.inv()*one_mat;

        w_mat = w_mat/sum(w_mat).val[0];

        w_mat = w_mat.t();

        int  x = key_points[icx].pt.x/5;
        int  y = key_points[icx].pt.y/5;

        p_llc_data->llc_mask[y][x] = mat_mask.at<uchar>(y,x);

        for (int i=0; i<knn; i++)
        {
            int  idx = matchesv[i].trainIdx;
            p_llc_data->llc_idx[y][x][i] = idx;
            p_llc_data->llc_weight[y][x][i] = w_mat.at<float>(0,i);
        }
    }
}

void  opencv_llc_bow_Descriptor(Mat &image, Mat &vocabulary,  vector<KeyPoint> &key_points, Mat &mat_llc_descriptor )
{
    //std::cout << "opencv_llc_bow_Descriptor" << std::endl;

    Mat descriptors;

    Ptr<DescriptorExtractor> extractor = DescriptorExtractor::create( "SURF" );

    extractor->compute( image, key_points, descriptors );

    int     knn = 5;
    float   fbeta = 1e-4;

    vector<vector<DMatch> > matches;

    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce" );
    //Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "FlannBased" );

    matcher->knnMatch( descriptors, vocabulary, matches, knn );

    for (int icx=0; icx<descriptors.rows; icx++)
    {
        vector<DMatch> &matchesv = matches[icx];

        Mat   des_mat_r1 = descriptors.row(icx);

        Mat   mat_cbknn;
        mat_cbknn.release();

        for (int i=0; i<knn; i++)
        {
            Mat  mat_idx = vocabulary.row(matchesv[i].trainIdx);

            mat_cbknn.push_back(mat_idx);
        }

        Mat  ll_mat = Mat::eye(knn, knn, CV_32FC1);
        Mat  z_mat = mat_cbknn - repeat(des_mat_r1, 5, 1);
        Mat  one_mat = Mat::ones(knn, 1, CV_32FC1);
        Mat  c_mat = z_mat*z_mat.t();

        float  ftrace = trace(c_mat).val[0];

        c_mat = c_mat + ll_mat*fbeta*ftrace;

        Mat  w_mat = c_mat.inv()*one_mat;

        w_mat = w_mat/sum(w_mat).val[0];

        w_mat = w_mat.t();

        for (int i=0; i<knn; i++)
        {
            mat_llc_descriptor.at<float>(icx, matchesv[i].trainIdx) = w_mat.at<float>(0,i);
        }
    }
}

void   list_images(vector<string>  &file_paths,  vector<string>  &image_paths)
{
    int  size_files = file_paths.size();

    for (int i=0; i<size_files; i++)
    {
        string   str_path = file_paths[i];

        string  str_ext = "";

        if(str_path.rfind('.') != string::npos)
        {
            str_ext = str_path.substr(str_path.rfind('.') + 1);
        }

        if (str_ext == "jpg" || str_ext == "jpeg" || str_ext == "png")  image_paths.push_back(str_path);
    }
}

void  list_dirs( const string& directory, vector<string>  &entries)
{
    char dir[MAX_PATH];
    string  str_dir = directory;

    strncpy(dir, str_dir.c_str(), MAX_PATH);

    DIR             *p_dir;
    DIR             *p_subdir;

    struct dirent *p_dirent;

    p_dir = opendir(dir);

    while((p_dirent = readdir(p_dir))!= NULL)
    {
        string  str_fn = p_dirent->d_name;
        string  str_path = directory + str_fn;

        p_subdir =  opendir(str_path.c_str());

        if (str_fn != "." && str_fn != ".."  && p_subdir)  entries.push_back(str_path);

        if (p_subdir) closedir(p_subdir);
    }
}

void  list_files(vector<string>  &dir_paths, vector<string>  &file_paths)
{
    int size_dirs = dir_paths.size();

    for (int i=0; i<size_dirs; i++)
    {
        string    dir_path = dir_paths[i];

        DIR             *p_dir;
        DIR             *p_subdir;

        struct dirent *p_dirent;

        p_dir = opendir(dir_path.c_str());

        while (p_dirent = readdir(p_dir))
        {
            string  str_fn = p_dirent->d_name;
            string  str_path = dir_path + "/" + str_fn;

            p_subdir =  opendir(str_path.c_str());

            if (str_fn != "." && str_fn != ".."   &&  p_subdir== NULL)  file_paths.push_back(str_path);

            if (p_subdir) closedir(p_subdir);
        }

        if (p_dir) closedir(p_dir);
    }
}


void  create_imgs_db(string  &sqlite_fn)
{
    const char *sql_create_table="create table imgs(id INTEGER PRIMARY KEY AUTOINCREMENT, imhash text, impath text, imw INTEGER, imh INTEGER, imhist blob)";
    char *errmsg = 0;
    int   ret = 0;

    sqlite3 *db = 0;
    ret = sqlite3_open(sqlite_fn.c_str(), &db);

    if(ret != SQLITE_OK)
    {
        fprintf(stderr,"Cannot open db: %s\n",sqlite3_errmsg(db));
        return;
    }

    printf("Open database\n");

    ret = sqlite3_exec(db, sql_create_table, NULL, NULL, &errmsg);

    if(ret != SQLITE_OK)
    {
        fprintf(stderr,"create table fail: %s\n",errmsg);
    }

    ret = sqlite3_exec(db, "CREATE INDEX imhash_idx ON imgs(imhash);", NULL, NULL, &errmsg);

    if(ret != SQLITE_OK)
    {
        fprintf(stderr,"create index fail: %s\n",errmsg);
    }

    ret = sqlite3_exec(db, "CREATE VIEW  imgs_view AS SELECT id, imhash, imhist FROM imgs;", NULL, NULL, &errmsg);

    if(ret != SQLITE_OK)
    {
        fprintf(stderr,"create view fail: %s\n",errmsg);
    }

    sqlite3_free(errmsg);
    sqlite3_close(db);

    printf("Close database\n");
}

bool  sort_match_imgs(string  &sqlite_fn, Mat &vocabulary, string &result_path, Mat &voc_matchs, Mat &main_hist01, Mat &key_hists01, std::vector<std::string> &img_match_hashs, multimap<double, string, greater<double> >  &map_hists)
{
    sqlite3_stmt *stmt;
    char  ca[255];

    char *errmsg = 0;
    int   ret_db = 0;

    sqlite3 *db = 0;
    ret_db = sqlite3_open_v2(sqlite_fn.c_str(), &db, SQLITE_OPEN_READONLY, NULL);

    for (int i=0; i<img_match_hashs.size(); i++)
    {
        string  str_hash02 = img_match_hashs[i];
        string  str_fn02;

        Mat  main_hist02;
        Mat  key_hists02;

        voc_db_mats(db, str_hash02, result_path, vocabulary, voc_matchs, str_fn02, main_hist02, key_hists02);

        if (key_hists01.rows == 0 || key_hists02.rows == 0) continue;

        double  distance_main = compareHist(main_hist01, main_hist02, CV_COMP_CORREL);

        //std::cout << "str_fn02 : " << str_fn02 << std::endl;

        double  average_distance;
        double  average_distance01;
        double  average_distance02;
        double  sum_distance01 = 0.0;
        double  sum_distance02 = 0.0;

        Ptr<DescriptorMatcher>  matcher_hists = DescriptorMatcher::create( "BruteForce" );

        vector<vector<DMatch> > matches_hists;

        int    knn=1;

        matcher_hists -> knnMatch( key_hists01, key_hists02, matches_hists, knn );

        float  dn01=0.0001;
        for (int i=0; i<key_hists01.rows; i++)
        {
            vector<DMatch> &matchesv = matches_hists[i];

            Mat  hist01 = key_hists01.row(i);

            Mat  hist02 = key_hists02.row(matchesv[0].trainIdx);

            double  distance_hist = compareHist(hist01, hist02, CV_COMP_CORREL);

            //if (distance_hist > 0.8*distance_main || distance_hist > 0.8)
            if ( distance_hist > 0.4)
            {
                sum_distance01 += distance_hist;
                dn01+=1.0;
            }
        }

        average_distance01 = sum_distance01/dn01;
        if (dn01 < 4.0) average_distance01 = 0.0;

        matches_hists.clear();

        matcher_hists -> knnMatch( key_hists02, key_hists01, matches_hists, knn );

        float  dn02=0.0001;
        for (int i=0; i<key_hists02.rows; i++)
        {
            vector<DMatch> &matchesv = matches_hists[i];

            Mat  hist02 = key_hists02.row(i);

            Mat  hist01 = key_hists01.row(matchesv[0].trainIdx);

            double  distance_hist = compareHist(hist01, hist02, CV_COMP_CORREL);

            //if (distance_hist > 0.8*distance_main || distance_hist > 0.8)
            if ( distance_hist > 0.4)
            {
                sum_distance02 += distance_hist;
                dn02+=1.0;
            }
        }

        average_distance02 = sum_distance02/dn02;
        if (dn02 < 4.0) average_distance02 = 0.0;

        average_distance = (average_distance01+average_distance02)/2.0;

        std::cout << str_hash02 << ": .............. " << std::endl;

        map_hists.insert(multimap<double, string, greater<double> >::value_type(average_distance, str_hash02));
    }

    //sqlite3_finalize(stmt);
    sqlite3_close(db);

    return true;
}

bool  read_db_map(string &sqlite_fn, map<string, string> &hash_fn_map)
{
    sqlite3_stmt  *stmt;
    char          ca[255];
    const  char   *zTail;

    char  *errmsg = 0;
    int    ret_db = 0;

    sqlite3 *db = 0;
    int  result = sqlite3_open_v2(sqlite_fn.c_str(), &db, SQLITE_OPEN_READONLY, NULL);

    result = sqlite3_prepare(db, "SELECT id, imhash, impath FROM imgs;", -1, &stmt, &zTail);

    result = sqlite3_step(stmt);

    while( result == SQLITE_ROW )
    {
        int id = sqlite3_column_int( stmt, 0 );
        const char* ch_hash = (char*)sqlite3_column_text( stmt, 1 );
        const char* ch_fn = (char*)sqlite3_column_text( stmt, 2 );

        string  str_hash = ch_hash;
        string  str_fn = ch_fn;

        hash_fn_map[str_hash] = str_fn;

        result = sqlite3_step(stmt);
    }

    map<string, string>::iterator  iter;

    for (iter = hash_fn_map.begin(); iter != hash_fn_map.end(); iter++)
    {
            string  str_hash = iter->first;
            string  str_fn = iter->second;

            std::cout << str_hash << " : " << str_fn << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return true;
}

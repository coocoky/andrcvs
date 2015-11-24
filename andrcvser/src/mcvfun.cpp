#include "mcvfun.h"
#include <stdio.h>

void  mcv_img_zoom(Mat  &image, Mat  &image_zoom, int  zwh)
{
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

    *p_l = zwh;
    float  zoom = *p_l*1.0/img_ls;
    *p_s = img_ss*zoom + 0.5;

    image_zoom = Mat::zeros( high01, width01, CV_8UC1);

    resize(image, image_zoom, Size(width01, high01));
}

void  draw_hist(char *win_name, Mat hist)
{
    int DRAW_H=600;
    Mat img_show = Mat::zeros(DRAW_H,  hist.cols*2, CV_8UC3);

    for (int i=0; i<hist.cols; i++)
    {
        line(img_show, Point(i*2, DRAW_H-200), Point(i*2, DRAW_H-(20.0*hist.at<float>(0,i)+0.5+200)), Scalar(255,255,255), 1);
    }

    namedWindow(win_name, 1);
    imshow(win_name, img_show);
}

void  filter_hist(Mat  &hist)
{
    for (int n=0; n<hist.cols; n++)
    {
        float  ve = hist.at<float>(0, n);

        if (ve < 0.0)  hist.at<float>(0, n) = 0.0;
    }

    int  idx_max = 0;
    //int  idx_min = 0;

    float  fmax = hist.at<float>(0, 0);
    //float  fmin = hist.at<float>(0, 0);

    float  sum = 0.0;

    for (int n=0; n<hist.cols; n++)
    {
        float ve = hist.at<float>(0, n);
        if (fmax < ve)
        {
            fmax = ve;
            idx_max = n;
        }
        //if (fmin > ve) {fmin = ve; idx_min = n;}

        sum += ve;
    }

    if ( fmax > 20.0*sum/hist.cols ) hist.at<float>(0, idx_max) = 20.0*sum/hist.cols;

    //hist.at<float>(0, idx_min) = 0.0;
}

bool  img_zoom_hist(Mat  &image,  Mat &vocabulary, Mat &voc_matchs, int  zoom_wh, Mat  &img_zoom, Mat &main_hist, LlcData  *p_llc_data, ZoomRoiData  *p_zoom_rois)
{
    int  wh = zoom_wh;

    mcv_img_zoom(image, img_zoom, wh);

    if (img_zoom.cols < MIN_ZOOM_WH || img_zoom.rows < MIN_ZOOM_WH) return false;

    int  m5_wh = wh/5;

    int  m5_w = img_zoom.cols/5;
    int  m5_h = img_zoom.rows/5;

    memset(p_llc_data, 0, sizeof(LlcData));

    memset(p_zoom_rois, 0, sizeof(ZoomRoiData));

    p_llc_data->m5_w = m5_w;
    p_llc_data->m5_h = m5_h;
    p_llc_data->m5_wh = m5_wh;

    //Ptr<DETECTOR> detector = DETECTOR::create();
    Ptr<FastFeatureDetector>  detector =  FastFeatureDetector::create(FAST_TH);

    Ptr<EXTRACTOR> extractor = EXTRACTOR::create(RADIUS);
    //Ptr<EXTRACTOR> extractor = EXTRACTOR::create(RADIUS*zoom);

    main_hist = Mat::zeros(1, SIZE_VOC, CV_32F);

    Mat    image_draw = Mat::zeros( img_zoom.rows, img_zoom.cols, CV_8UC1);

    vector<KeyPoint>  key_points;
    vector<KeyPoint>  key_points01;

    detector->detect( img_zoom, key_points01);

    for (int i=0; i<key_points01.size(); i++)
    {
        Point  point(key_points01[i].pt.x, key_points01[i].pt.y);

        circle( image_draw, point, 10, Scalar( 255, 255, 255 ), -1);
    }


    for (int my=3; my<=m5_h-3; my++)
    {
        for (int mx=3; mx<=m5_w-3; mx++)
        {
            if (image_draw.at<uchar>(5*my, 5*mx) > 200 )
            {
                KeyPoint  key_point;

                key_point.pt.x = 5*mx;
                key_point.pt.y = 5*my;

                key_points.push_back(key_point);
            }
        }
    }

    Mat  descriptors;
    Mat  llc_descriptor = Mat::zeros(1, VOCA_COLS, CV_32F);

    extractor -> compute( img_zoom, key_points, descriptors );

    int     knn = KNN;
    float   fbeta = 1e-4;

    vector<vector<DMatch> > matches;

    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce" );
    //Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "FlannBased" );

    //double time_b = (double)getTickCount();

    matcher -> knnMatch( descriptors, vocabulary, matches, knn );

    //double time_e = (double)getTickCount();
    //double time_hist = (time_e - time_b)/getTickFrequency()*1000.0;
    //std::cout << "time_hist : " << time_hist << std::endl;

    Mat  des_mat_r01;
    for (int icx=0; icx<descriptors.rows; icx++)
    {
        des_mat_r01 = descriptors.row(icx);

        vector<DMatch> &matchesv1 = matches[icx];

        Mat  mat_cbknn;

        mat_cbknn.release();


        for (int i=0; i<knn; i++)
        {
            Mat  mat_idx01 = vocabulary.row(matchesv1[i].trainIdx);

            mat_cbknn.push_back(mat_idx01);
        }
        //std::cout << "mat_cbknn size : " << mat_cbknn.rows << "  " << mat_cbknn.cols << std::endl;

        Mat  ll_mat = Mat::eye(knn, knn, CV_32FC1);
        Mat  z_mat = mat_cbknn - repeat(des_mat_r01, 5, 1);
        Mat  one_mat = Mat::ones(knn, 1, CV_32FC1);
        Mat  c_mat = z_mat*z_mat.t();

        float  ftrace = trace(c_mat).val[0];

        c_mat = c_mat + ll_mat*fbeta*ftrace;

        Mat  w_mat = c_mat.inv()*one_mat;

        w_mat = w_mat/sum(w_mat).val[0];

        w_mat = w_mat.t();

        for (int i=0; i<knn; i++)
        {
            llc_descriptor.at<float>(0, matchesv1[i].trainIdx) += w_mat.at<float>(0,i);
        }

        int  mx = key_points[icx].pt.x/5;
        int  my = key_points[icx].pt.y/5;

        p_llc_data->llc_mask[my][mx] = 255;

        for (int i=0; i<knn; i++)
        {
            int  idx = matchesv1[i].trainIdx;
            p_llc_data->llc_idx[my][mx][i] = idx;
            p_llc_data->llc_weight[my][mx][i] = w_mat.at<float>(0,i);
        }
    }

    llc_descriptor = llc_descriptor/(descriptors.rows*1.0)*200.0;

    int  voc_knn = voc_matchs.cols;

    for (int n=0; n<voc_matchs.rows; n++)
    {
        for (int i=0; i<voc_knn; i++)
        {
            int  idx = voc_matchs.at<int>(n, i);

            main_hist.at<float>(0, n) += llc_descriptor.at<float>(0, idx);
        }
    }

    int  roi_m5_wh=ROI_WH/5;
    //int  roi_m5_wh=240/5;

    for (int my0=3; my0<=m5_h-roi_m5_wh-3; my0+=2)
    {
        for (int mx0=3; mx0<=m5_w-roi_m5_wh-3; mx0+=2)
        {
            Mat   desc_hist = Mat::zeros(1, VOCA_COLS, CV_32F);

            int  num_mask = 0;

            for (int my=my0; my<=my0+roi_m5_wh; my++)
            {
                for (int mx=mx0; mx<=mx0+roi_m5_wh; mx++)
                {
                    if (p_llc_data->llc_mask[my][mx] <= 200) continue;

                    for (int n=0; n<5; n++)
                    {
                        int idx = p_llc_data->llc_idx[my][mx][n]+0.5;
                        desc_hist.at<float>(0, idx) += p_llc_data->llc_weight[my][mx][n];
                    }

                    num_mask++;
                }
            }

            desc_hist = desc_hist/(1.0*num_mask)*200.0;

            int  voc_knn = voc_matchs.cols;

            //std::cout << "voc_knn : " << voc_knn << std::endl;

            Mat  voc_hist = Mat::zeros(1, SIZE_VOC, CV_32F);

            for (int n=0; n<voc_matchs.rows; n++)
            {
                for (int i=0; i<voc_knn; i++)
                {
                    int  idx = voc_matchs.at<int>(n, i);

                    //voc_hist.at<float>(0, n) += desc_hist.at<float>(0, idx);
                    p_zoom_rois->float_hists[my0][mx0][n] += desc_hist.at<float>(0, idx);
                }
            }

            p_zoom_rois->num_mask[my0][mx0] = num_mask;
            if (num_mask > MIN_ROIS_KEYS) { p_zoom_rois->mask[my0][mx0] = 255;}
        }
    }

    return  true;
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

    int  ret_db = sqlite3_step(stmt);

    sqlite3_reset(stmt);
    //sqlite3_finalize(stmt);
    //sqlite3_free(errmsg);
}

void  opencv_llc_bow_mats(Mat &image, Mat &vocabulary, Mat &mat_mask, Mat  &descriptors, float radius, LlcData *p_llc_data)
{
    //std::cout << "opencv_llc_bow_Descriptor" << std::endl;

    //Ptr<DETECTOR> detector = DETECTOR::create();
    Ptr<FastFeatureDetector>  detector =  FastFeatureDetector::create(FAST_TH);

    //Ptr<SURF> extractor = SURF::create();
    Ptr<EXTRACTOR> extractor = EXTRACTOR::create(radius);

    vector<KeyPoint> key_points;
    vector<KeyPoint> key_points01;

    KeyPoint   key_point;

    for (int y=2; y<mat_mask.rows-2; y++)
    {
        for (int x=2; x<mat_mask.cols-2; x++)
        {
            key_point.pt.x = 5*x;
            key_point.pt.y = 5*y;
            key_point.angle = 0.0;
            key_point.size = radius*2;

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

    Ptr<EXTRACTOR> extractor = EXTRACTOR::create(RADIUS);

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

double  compare_class_hists(set<string>  &class_names, map<string, Mat>  &map_class_hists01, map<string, Mat>  &map_class_hists02)
{
    map<string, double>   map_class_distance;
    map<string, double>   map_class_weight;

    for (set<string>::iterator iter = class_names.begin(); iter != class_names.end(); iter++)
    {
        string   class_name = *iter;

        map_class_distance[class_name] = 0.0;
        map_class_weight[class_name] = 0.0;
    }

    Ptr<DescriptorMatcher>  matcher = DescriptorMatcher::create( "BruteForce" );

    int  sum_num = 0;

    for (set<string>::iterator iter = class_names.begin(); iter != class_names.end(); iter++)
    {
        string   class_name = *iter;

        Mat      &hists01 = map_class_hists01[class_name];
        Mat      &hists02 = map_class_hists02[class_name];

        if (hists01.empty() || hists02.empty() ) continue;

        vector<vector<DMatch> > matches;
        matches.clear();

        matcher -> knnMatch( hists01, hists02, matches, 1 );

        double  sum_dist = 0.0;
        double  ave_dist = 0.0;

        int  num=0;

        for (int n=0; n<hists01.rows; n++)
        {
            vector<DMatch> &matchesv1 = matches[n];

            Mat  hist01 = hists01.row(n);
            Mat  hist02 = hists02.row(matchesv1[0].trainIdx);

            double   distance = compareHist(hist01, hist02, CV_COMP_CORREL);

            if (distance < 0.0) distance = 0.0;

            if (distance > 0.2)
            {
                num++;
                sum_dist += distance;
            }
        }

        if (num > 0)
        {
            ave_dist = sum_dist/num;
        }
        else
        {
            ave_dist = 0.0;
        }

        sum_num += num;

        map_class_distance[class_name] = ave_dist;
        map_class_weight[class_name] = num;
    }

    for (set<string>::iterator iter = class_names.begin(); iter != class_names.end(); iter++)
    {
        string   class_name = *iter;

        //std::cout << class_name << " distance: " << map_class_distance[class_name] << "    num: " << map_class_weight[class_name] << std::endl;

        if (sum_num > 10)
        {
            map_class_weight[class_name] = map_class_weight[class_name] / sum_num;
        }
        else
        {
            map_class_weight[class_name] = 0.0;
        }
    }

    //std::cout << "*********************************************************************" << std::endl;

    double  class_distance = 0.0;

    for (set<string>::iterator iter = class_names.begin(); iter != class_names.end(); iter++)
    {
        string   class_name = *iter;

        class_distance += map_class_distance[class_name] * map_class_weight[class_name];
    }

    //std::cout << "class_distance: " << class_distance << std::endl;

    return   class_distance;
}

void  class_bow_save(std::string &file_hash, string  &str_path, set<string>  &class_names, LlcData  *p_llc_data, map<string, VecPoint >  &map_class_rois_points)
{
    string   str_bow_fn = str_path;

    str_bow_fn += "bow_";
    str_bow_fn += file_hash;
    str_bow_fn += ".xml.gz";

    FileStorage fs_bow( str_bow_fn, FileStorage::WRITE );

    if ( fs_bow.isOpened() )
    {
        fs_bow << "mwh" << p_llc_data->m5_wh;
        fs_bow << "mw" << p_llc_data->m5_w;
        fs_bow << "mh" << p_llc_data->m5_h;

        vector<uchar>  llc_masks;
        llc_masks.resize(MWH*MWH);
        char  *p_data = (char*)(&llc_masks[0]);
        memmove(p_data,  (char*)(p_llc_data->llc_mask), MWH*MWH*sizeof(uchar));
        fs_bow << "llc_masks" << llc_masks;

        vector<float>  llc_idxs;
        llc_idxs.resize(MWH*MWH*KNN);
        p_data = (char*)(&llc_idxs[0]);
        memmove(p_data,  (char*)(p_llc_data->llc_idx), MWH*MWH*KNN*sizeof(float));
        fs_bow << "llc_idxs" << llc_idxs;

        vector<float>  llc_weights;
        llc_weights.resize(MWH*MWH*KNN);
        p_data = (char*)(&llc_weights[0]);
        memmove(p_data,  (char*)(p_llc_data->llc_weight), MWH*MWH*KNN*sizeof(float));
        fs_bow << "llc_weights" << llc_weights;

        for (set<string>::iterator iter = class_names.begin(); iter != class_names.end(); iter++)
        {
            string     class_name = *iter;

            VecPoint    points = map_class_rois_points[class_name];

            fs_bow << class_name << points;
        }
    }

    fs_bow.release();
}

void  read_class_db(string &df_path,  set<string>  &class_names,  map<string, Mat>  &map_class_vocs, vector<string>  &strs_rois_class, Mat  &class_hists)
{
    vector<string>   str_files;

    vector<string>   dir_paths;
    dir_paths.push_back(df_path);

    list_files(dir_paths,  str_files);

    for (int n=0; n<str_files.size(); n++)
    {
        string  str_fn = str_files[n];

        int  pos01 = df_path.length();

        int  pos02 = str_fn.find(".xml.gz");

        string  class_name = str_fn.substr(pos01+1, pos02-pos01-1);

        class_names.insert(class_name);

        string  str_fs_path = str_fn;

        //std::cout << class_name << std::endl;

        Mat   hists;
        hists.release();

        FileStorage fs_hists( str_fs_path, FileStorage::READ );
        if ( fs_hists.isOpened() )  fs_hists["class_hists"] >> hists;
        fs_hists.release();

        for (int n=0; n<hists.rows; n++)
        {
            Mat   hist = hists.row(n);
            strs_rois_class.push_back(class_name);
            class_hists.push_back(hist);
            map_class_vocs[class_name].push_back(hist);
        }
    }
}

bool  img_zoom_class_hists(Mat &image, Mat &vocabulary, Mat &voc_matchs, int  zoom_wh, set<string>  &class_names,  vector<string>  &strs_rois_class,
                           Mat   &class_hists,  Mat &main_hist,  map<string, Mat>  &map_class_hists)
{
    map<string, double>   map_class_distance;

    for (set<string>::iterator iter = class_names.begin(); iter != class_names.end(); iter++)
    {
        string   class_name = *iter;

        map_class_distance[class_name] = 0.0;

        Mat      hist;

        map_class_hists[class_name] = hist.clone();
    }

    //Mat  main_mask;
    Mat   img_zoom;

    map<string, double>    map_class_distance_locale;


    for (map<string, double>::iterator iter = map_class_distance.begin(); iter != map_class_distance.end(); iter++)
    {
        string   str_class_name = iter->first;

        map_class_distance_locale[str_class_name] = 0.0;
    }


    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce" );

    Mat               roi_hists;
    vector<Point>     xy_hists;
    int               num_mask;

    roi_hists.release();
    xy_hists.clear();

    num_mask = 0;

    double time_b = (double)getTickCount();

    ZoomRoiData  *p_zoom_rois = new ZoomRoiData();

    LlcData   *p_llc_data = new LlcData();

    bool  ret = img_zoom_hist(image, vocabulary, voc_matchs, zoom_wh, img_zoom, main_hist, p_llc_data, p_zoom_rois);

    if (ret == false) {  delete  p_zoom_rois;  delete  p_llc_data;  return  false; }

    filter_hist(main_hist);

    int  m5_w = img_zoom.cols/5;
    int  m5_h = img_zoom.rows/5;

    for (int my=0; my<=m5_h; my++)
    {
        for (int mx=0; mx<=m5_w; mx++)
        {
            Mat    hist = Mat::zeros(1, SIZE_VOC, CV_32F);

            Point  point(5*mx, 5*my);

            if (p_zoom_rois->mask[my][mx] == 255)
            {
                num_mask++;

                for (int n=0; n<SIZE_VOC; n++)
                {
                    hist.at<float>(0, n) = p_zoom_rois->float_hists[my][mx][n];
                }

                filter_hist(hist);

                roi_hists.push_back(hist);
                xy_hists.push_back(point);
            }
        }
    }

    int  knn=5;

    vector<vector<DMatch> > matches;

    matcher -> knnMatch( roi_hists, class_hists, matches, knn );

    double time_e = (double)getTickCount();
    double time_hists = (time_e - time_b)/getTickFrequency()*1000.0;
    std::cout << "time_hists : " << time_hists << std::endl;

    for (int n=0; n<roi_hists.rows; n++)
    {
        vector<DMatch> &matchesv1 = matches[n];

        Mat  hist01 = roi_hists.row(n);

        Point  point = xy_hists[n];

        set<string>  set_class_names;
        set_class_names.clear();

        for (int k=0; k<knn; k++)
        {
            Mat      hist02 = class_hists.row(matchesv1[k].trainIdx);

            double   distance = compareHist(hist01, hist02, CV_COMP_CORREL);

            string   class_name = strs_rois_class[matchesv1[k].trainIdx];

            //if (distance > 0.40)
            if (distance > 0.50)
            {
                map_class_distance_locale[class_name] += distance;
                set_class_names.insert(class_name);
            }
        }

        for (set<string>::iterator iter = set_class_names.begin(); iter != set_class_names.end(); iter++)
        {
            string   class_name = *iter;

            Mat      &hists = map_class_hists[class_name];

            hists.push_back(hist01);
        }
    }

    for (map<string, double>::iterator iter = map_class_distance.begin(); iter != map_class_distance.end(); iter++)
    {
        string   str_class_name = iter->first;
        double   distance_locale = map_class_distance_locale[str_class_name];

        map_class_distance[str_class_name] += distance_locale;
    }

    delete  p_zoom_rois;
    delete  p_llc_data;

    return  true;
}

bool  img_zoom_class_rois(Mat &image, Mat &vocabulary, Mat &voc_matchs, int  zoom_wh, set<string>  &class_names,  vector<string>  &strs_rois_class,
                           Mat   &class_hists,  Mat &main_hist, LlcData  *p_llc_data,  map<string, VecPoint >  &map_class_rois_points)
{
    //memset(p_llc_data, 0, sizeof(LlcData));

    ZoomRoiData  *p_zoom_rois = new ZoomRoiData();

    for (set<string>::iterator iter = class_names.begin(); iter != class_names.end(); iter++)
    {
        string   class_name = *iter;

        //map_class_distance[class_name] = 0.0;

        VecPoint      rois_points;

        map_class_rois_points[class_name] = rois_points;
    }

    //Mat  main_mask;
    Mat   img_zoom;


    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce" );


    Mat               roi_hists;
    vector<Point>     xy_hists;
    int               num_mask;

    roi_hists.release();
    xy_hists.clear();

    num_mask = 0;

    double time_b = (double)getTickCount();

    bool ret = img_zoom_hist(image, vocabulary, voc_matchs, zoom_wh, img_zoom, main_hist, p_llc_data, p_zoom_rois);

    if (ret == false) {  delete  p_zoom_rois;  return  false; }

    filter_hist(main_hist);

    int  m5_w = img_zoom.cols/5;
    int  m5_h = img_zoom.rows/5;

    for (int my=0; my<=m5_h; my++)
    {
        for (int mx=0; mx<=m5_w; mx++)
        {
            Mat    hist = Mat::zeros(1, SIZE_VOC, CV_32F);

            Point  point(5*mx, 5*my);

            if (p_zoom_rois->mask[my][mx] == 255)
            {
                num_mask++;

                for (int n=0; n<SIZE_VOC; n++)
                {
                    hist.at<float>(0, n) = p_zoom_rois->float_hists[my][mx][n];
                }

                filter_hist(hist);

                roi_hists.push_back(hist);
                xy_hists.push_back(point);
            }
        }
    }

    int  knn=5;

    vector<vector<DMatch> > matches;

    matcher -> knnMatch( roi_hists, class_hists, matches, knn );

    double time_e = (double)getTickCount();
    double time_class_bow = (time_e - time_b)/getTickFrequency()*1000.0;
    std::cout << "time_class_bow : " << time_class_bow << std::endl;

    for (int n=0; n<roi_hists.rows; n++)
    {
        vector<DMatch> &matchesv1 = matches[n];

        Mat  hist01 = roi_hists.row(n);

        Point  point = xy_hists[n];

        set<string>  set_class_names;
        set_class_names.clear();

        for (int k=0; k<knn; k++)
        {
            Mat      hist02 = class_hists.row(matchesv1[k].trainIdx);

            double   distance = compareHist(hist01, hist02, CV_COMP_CORREL);

            string   class_name = strs_rois_class[matchesv1[k].trainIdx];

            //if (distance > 0.40)
            if (distance > 0.50) set_class_names.insert(class_name);
        }

        for (set<string>::iterator iter = set_class_names.begin(); iter != set_class_names.end(); iter++)
        {
            string   class_name = *iter;

            VecPoint   &m5_points = map_class_rois_points[class_name];

            Point   m5_point;

            m5_point.x = point.x/5;
            m5_point.y = point.y/5;

            m5_points.push_back(m5_point);
        }
    }

    delete  p_zoom_rois;

    return  true;
}

void  llc_data_to_class_hists(set<string>  &class_names, Mat &voc_matchs, LlcData  *p_llc_data, map<string, VecPoint >  &map_class_rois_points,  map<string, Mat>  &map_class_hists)
{
    for (set<string>::iterator iter = class_names.begin(); iter != class_names.end(); iter++)
    {
        string   class_name = *iter;

        Mat      hist;

        map_class_hists[class_name] = hist;
    }

    ZoomRoiData   *p_zoom_rois = new ZoomRoiData();

    memset(p_zoom_rois, 0, sizeof(ZoomRoiData));

    int  m5_w = p_llc_data->m5_w;
    int  m5_h = p_llc_data->m5_h;
    int  m5_wh = p_llc_data->m5_wh;

    int  roi_m5_wh=100/5;

    for (int my0=3; my0<=m5_h-roi_m5_wh-3; my0+=1)
    {
        for (int mx0=3; mx0<=m5_w-roi_m5_wh-3; mx0+=1)
        {
            Mat   desc_hist = Mat::zeros(1, VOCA_COLS, CV_32F);

            int  num_mask = 0;

            for (int my=my0; my<=my0+roi_m5_wh; my++)
            {
                for (int mx=mx0; mx<=mx0+roi_m5_wh; mx++)
                {
                    if (p_llc_data->llc_mask[my][mx] <= 200) continue;

                    for (int n=0; n<KNN; n++)
                    {
                        int idx = p_llc_data->llc_idx[my][mx][n]+0.5;
                        desc_hist.at<float>(0, idx) += p_llc_data->llc_weight[my][mx][n];
                    }

                    num_mask++;
                }
            }

            desc_hist = desc_hist/(1.0*num_mask)*200.0;

            int  voc_knn = voc_matchs.cols;

            //std::cout << "voc_knn : " << voc_knn << std::endl;

            Mat  voc_hist = Mat::zeros(1, SIZE_VOC, CV_32F);

            for (int n=0; n<voc_matchs.rows; n++)
            {
                for (int i=0; i<voc_knn; i++)
                {
                    int  idx = voc_matchs.at<int>(n, i);

                    //voc_hist.at<float>(0, n) += desc_hist.at<float>(0, idx);
                    p_zoom_rois->float_hists[my0][mx0][n] += desc_hist.at<float>(0, idx);
                }
            }

            p_zoom_rois->num_mask[my0][mx0] = num_mask;
            if (num_mask > 160) { p_zoom_rois->mask[my0][mx0] = 255;}
        }
    }

    for (set<string>::iterator iter = class_names.begin(); iter != class_names.end(); iter++)
    {
        string   class_name = *iter;

        VecPoint  m5_points = map_class_rois_points[class_name];

        for (int n=0; n<m5_points.size(); n++)
        {
            Point  m5_point = m5_points[n];

            Mat    voc_hist = Mat::zeros(1, SIZE_VOC, CV_32F);

            for (int i=0; i<voc_hist.cols; i++)
            {
                voc_hist.at<float>(0, i) = p_zoom_rois->float_hists[m5_point.y][m5_point.x][i];
            }

            map_class_hists[class_name].push_back(voc_hist);
        }
    }

    delete  p_zoom_rois;
}

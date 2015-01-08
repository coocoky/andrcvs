#include "mcvfun.h"
#include <stdio.h>

Mat  surf_hist_120(Mat  &image, Ptr<DescriptorExtractor>  &extractor, float angle, int  size_surf)
{
    vector<KeyPoint> key_points;

    KeyPoint   key_point;

    for (int y=15;  y<=120-15;  y+=30)
    {
        for(int x=15;  x<=120-15; x+=30)
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

bool  calc_hist(Mat  &image, Mat &vocabulary,  Mat &sum_query_descriptor)
{
    vector<KeyPoint> keyPoints;
    //vector<KeyPoint> keyPoints01;

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

    resize(image, image_zoom, Size(width01, high01));

    if (high01 < ZOOM_SIZE/4 || width01 < ZOOM_SIZE/4) return false;

    /*
    detector->detect(image_zoom, keyPoints01);
    for(uint32_t i=0; i<keyPoints01.size(); i++)
    {
        KeyPoint  myPoint;

        myPoint = keyPoints01[i];

        if (myPoint.size >= MIN_KPS) keyPoints.push_back(myPoint);
    }
    */


    for (int y=10;  y<=high01-10;  y+=STEP)
    {
        for(int x=10;  x<=width01-10; x+=STEP)
        {
            KeyPoint  key_point;

            key_point.pt.x = x;
            key_point.pt.y = y;
            key_point.angle = 0;
            key_point.size = 20;

            keyPoints.push_back(key_point);
        }
    }


    Mat  mat_query_descriptor;

    int   size_keys = keyPoints.size();

    mat_query_descriptor = Mat::zeros(size_keys, VOCA_COLS, CV_32F);

    opencv_llc_bow_Descriptor( image_zoom, vocabulary, keyPoints, mat_query_descriptor);

    for (int i=0; i<size_keys; i++)
    {
        sum_query_descriptor  =  sum_query_descriptor + mat_query_descriptor.row(i);
    }

    sum_query_descriptor = sum_query_descriptor/(size_keys*1.0);

    sum_query_descriptor = sum_query_descriptor*1000.0;

    return  true;
}

bool  voc_hist(Mat  &img_src,  Mat &vocabulary,  Mat &voc_matchs, Mat &voc_hist)
{
    Mat sum_query_descriptor = Mat::zeros(1, VOCA_COLS, CV_32F);

    bool ret = calc_hist(img_src, vocabulary, sum_query_descriptor);

    if (ret == false) return ret;

    int  knn = voc_matchs.cols;

    for (int n=0; n<voc_matchs.rows; n++)
    {
        for (int i=0; i<3; i++)
        {
            int  idx01 = voc_matchs.at<int>(n, i);

            voc_hist.at<float>(0, n) += sum_query_descriptor.at<float>(0, idx01);

        }
    }

    return true;
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
            //sum_llc_descriptor.at<float>(0, matchesv1[i].trainIdx) += w_mat.at<float>(0,i);
            mat_llc_descriptor.at<float>(icx, matchesv1[i].trainIdx) = w_mat.at<float>(0,i);
        }
    }

    //sum_llc_descriptor = sum_llc_descriptor/(descriptors.rows*1.0);
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
    const char *sql_create_table="create table imgs(id INTEGER PRIMARY KEY AUTOINCREMENT, path text, hist blob)";
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

    sqlite3_free(errmsg);
    sqlite3_close(db);

    printf("Close database\n");
}

#include "MyApp.h"

#include "TranDataHandler.h"
#include "mcvfun.h"

TranDataHandler::TranDataHandler(string voc_fn, string  imgs_path, string result_path)
{
    // Your initialization goes here

    id_imgs = 1;

    std::string   vocabulary_file =  voc_fn;
    std::string   voc_matchs_fn = result_path + "/voc_matchs.xml.gz";

    m_result_path = result_path;

    sqlite_fn = result_path + "/imgs.db";

    m_imgs_path = imgs_path;
    //m_str_charset = str_charset;

    FileStorage fs( vocabulary_file, FileStorage::READ );
    if ( fs.isOpened() )  fs["vocabulary"] >> vocabulary;

    std::cout << "vocabulary_file :  " << vocabulary_file << std::endl;
    std::cout << "vocabulary rows cols = " << vocabulary.rows << "  " << vocabulary.cols << std::endl;

    FileStorage fs_voc( voc_matchs_fn, FileStorage::READ );
    if ( fs_voc.isOpened() )  fs_voc["voc_matchs"] >> voc_matchs;

    std::cout << "voc_matchs rows cols = " << voc_matchs.rows << "  " << voc_matchs.cols << std::endl;

    int  pos = voc_fn.rfind("/");
    string  str_class_dir = voc_fn.substr(0, pos);
    str_class_dir += "/class";

    read_class_db(str_class_dir,  class_names, map_class_vocs, strs_rois_class, class_hists);

    std::cout << "strs_rois_class.size : " << strs_rois_class.size() << std::endl;
    std::cout << "class_hists.rows : " << class_hists.rows << std::endl;


    string        img_fn;
    string        img_path;
    char          ch_buf[1024];

    sqlite3 *db = NULL;

    sqlite3_stmt *stmt;

    const char *zTail;
    char    *zErrorMsg;

    int result = sqlite3_open_v2(sqlite_fn.c_str(), &db, SQLITE_OPEN_READONLY, NULL);

    if (SQLITE_OK != result)
    {
        std::cout << "sqlite3_open error !!" << std::endl;

        //return;
        //exit(-1);
    }

    //result = sqlite3_exec( db , "begin transaction" , 0 , 0 , &zErrorMsg );

    result = sqlite3_prepare(db, "SELECT id, imhash, imhist, impath FROM imgs;", -1, &stmt, &zTail);
    //result = sqlite3_prepare(db,"SELECT * FROM imgs WHERE id=3;", -1, &stmt, &zTail);
    //result = sqlite3_prepare(db,"SELECT id, value, weight FROM test;", -1, &stmt, &zTail);

    result = sqlite3_step(stmt);

    while( result == SQLITE_ROW )
    {
        int id = sqlite3_column_int( stmt, 0 );
        const char* ch_hash = (char*)sqlite3_column_text( stmt, 1 );

        const void   *p_buf = sqlite3_column_blob(stmt, 2);
        int  size_buf = sqlite3_column_bytes(stmt, 2);

        Mat hist = Mat::zeros(1, SIZE_VOC, CV_32F);

        vector<float>   hist_data;
        hist_data.resize(hist.cols);

        char  *p_data = (char*)(&hist_data[0]);

        memmove(p_data,  p_buf, size_buf);

        for (int i=0; i<hist.cols; i++)
        {
            hist.at<float>(0, i) = hist_data[i];
        }

        const char* ch_impath = (char*)sqlite3_column_text( stmt, 3 );

        string   str_hash = ch_hash;
        string   str_fn   = ch_impath;

        m_hash_fns[ch_hash] = str_fn;

        //std::cout << "id : " << id << "  path : " << path << " hist.rows :" << hist.cols << std::endl;

        m_hashs.push_back(ch_hash);
        main_hists.push_back(hist);

        result = sqlite3_step(stmt);
    }

    //result = sqlite3_exec ( db , "commit transaction" , 0 , 0 , &zErrorMsg );

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    std::cout << "main_hists rows cols = " << main_hists.rows << "  " << main_hists.cols << std::endl;
}

void TranDataHandler::hello_string(std::string& _return, const std::string& para)
{
    // Your implementation goes here

    _return = "hello_string cvrpc para : ";

    _return += para;

    //printf("hello_string\n");

    std::cout << _return << std::endl;
}

void TranDataHandler::opencv_rpc(std::string& _return, const std::string& fun_name, const std::vector<std::string> & pa, const std::string& in_data)
{

    // Your implementation goes here
    printf("opencv_rpc\n");
}

void TranDataHandler::read_image(std::string& _return, const std::string &file_hash, const std::map<std::string, std::string> &pa)
{
    // Your implementation goes here
    //printf("read_image\n");

    string  str_rewh = pa.at("wh");
    string  str_quality_jpg = pa.at("jpg");

    int     rewh = atoi(str_rewh.c_str());
    int     qua_jpg = atoi(str_quality_jpg.c_str());

    //std::cout << "rewh  qua_jpg : " << rewh << "  " << qua_jpg << std::endl;

    /*
    string  str_fn = "";

    if (m_str_charset != "utf-8")
    {
        CodeConverter  cc("utf-8", m_str_charset.c_str());

        cc.convert_string(file_name, str_fn);
    }
    else
    {
        str_fn = file_name;
    }
    */

    string  str_fn = m_hash_fns[file_hash];
    string  str_path = m_imgs_path + str_fn;

    Mat  img_src = imread(str_path, CV_LOAD_IMAGE_COLOR);

    //string  str_txt = str_fn.substr(m_imgs_path.length());
    //std::cout << "read image :  " << img_src.cols << "  " << img_src.rows << "  " << str_fn << std::endl;

    char   chbuf[1024];

    snprintf(chbuf, 1000, "read image :  %d  %d  %s", img_src.cols, img_src.rows, str_fn.c_str());

    chbuf[1000] = '\0';

    wxThreadEvent event( wxEVT_THREAD, WORKER_EVENT );

    event.SetString(chbuf);

    //MyApp  &app = wxGetApp();
    //wxQueueEvent(&app, event.Clone());

    if (img_src.data == 0 || img_src.rows == 0 || img_src.cols == 0)
    {
        _return.clear();
        return;
    }

    int    width01;
    int    high01;

    int    *p_l = &width01;
    int    *p_s = &high01;

    int    img_ls = img_src.cols;
    int    img_ss = img_src.rows;

    if (img_src.cols < img_src.rows)
    {
        p_s =  &width01;
        p_l =  &high01;
        img_ls = img_src.rows;
        img_ss = img_src.cols;
    }

    *p_l = rewh;
    float  zoom = *p_l*1.0/img_ls;
    *p_s = img_ss*zoom + 0.5;

    Mat    image_zoom = Mat::zeros( high01, width01, CV_8UC1);

    resize(img_src, image_zoom, Size(width01, high01), INTER_CUBIC);

    vector<uchar>   buf_img;
    vector<int>     params;

    buf_img.clear();
    params.push_back( cv::IMWRITE_JPEG_QUALITY );
    //params.push_back( 75 );
    params.push_back( qua_jpg );

    imencode( ".jpg", image_zoom, buf_img, params );

    int  size_buf = buf_img.size();
    char   *p_ch = (char*)(&buf_img[0]);

    _return.assign(p_ch, size_buf);
}

void TranDataHandler::image_match(std::vector<std::string> &_return, const std::string &img_data, const std::vector<std::string> &pa)
{
    // Your implementation goes here
    //printf("image_match\n");

    //std::cout << "image match ..." << std::endl;

    //erase();

    MyApp  &app = wxGetApp();

    wxThreadEvent  event_cls( wxEVT_THREAD, WORKER_CLS );
    wxQueueEvent(&app, event_cls.Clone());


    //wprintw(stdscr, "%s\n", "image match ..." );

    double time_b = (double)getTickCount();

    vector<uchar>   buf_img;

    int  size_str = img_data.size();

    buf_img.resize(size_str);

    char     *p_buf = (char*)(&buf_img[0]);
    char     *p_str = (char*)img_data.c_str();

    memmove(p_buf, p_str, size_str);

    Mat img_dst, img_gray, detected_edges;

    Mat image01 = imdecode(buf_img, CV_LOAD_IMAGE_GRAYSCALE);
    //Mat image = imdecode(buf_img, CV_LOAD_IMAGE_COLOR);

    Mat  main_hist01;

    map<string, Mat>      map_class_hists01;
    map_class_hists01.clear();

    //LlcData  *p_llc_data01 = new LlcData;

    img_zoom_class_hists(image01, vocabulary, voc_matchs, ZOOM_SIZE, class_names, strs_rois_class, class_hists,
                            main_hist01, map_class_hists01);

    int     mknn = 32;
    //float   fbeta = 1e-4;

    vector<vector<DMatch> > matches;

    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce" );
    //Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "FlannBased" );

    matcher -> knnMatch( main_hist01, main_hists, matches, mknn );

    multimap<double, string, greater<double> >  map_hists_hashs;

    vector<string>  hashs_knn;

    for (int n=0; n<mknn; n++)
    {
        vector<DMatch>  &matchesv = matches[0];

        int       midx = matchesv[n].trainIdx;
        string    str_hash = m_hashs[midx];
        Mat       hist_db = main_hists.row(midx);

        double    distance = compareHist(main_hist01, hist_db, CV_COMP_CORREL);

        if (distance > 0.20)
        {
            hashs_knn.push_back(str_hash);

            map_hists_hashs.insert(multimap<double, string, greater<double> >::value_type(distance, str_hash));
        }

    }

    double time_e = (double)getTickCount();
    double time_knn = (time_e - time_b)/getTickFrequency()*1000.0;

    char   chbuf[1024];
    snprintf(chbuf, 1000, "image match :  %f ms", time_knn);
    chbuf[1000] = '\0';

    wxThreadEvent event( wxEVT_THREAD, WORKER_EVENT );
    event.SetString(chbuf);

    //MyApp  &app = wxGetApp();
    //wxQueueEvent(&app, event.Clone());

    set<string>       fn_set;
    map<int, string>  fn_map;

    int  idx_map = 0;

    multimap<double, string, greater<double> >  map_class_distance_hashs;

    multimap<double, string, greater<double> >::iterator  iter;

    for (iter = map_hists_hashs.begin(); iter != map_hists_hashs.end(); iter++)
    {
        double  distance_hists = iter->first;
        string  str_hash_img = iter->second;

        string  str_fn = m_hash_fns[str_hash_img];
        string  str_path = m_imgs_path + str_fn;

        LlcData  *p_llc_data = new LlcData();
        memset(p_llc_data, 0, sizeof(LlcData));

        map<string, VecPoint >     map_class_rois_points;
        map_class_rois_points.clear();

        string   str_bow_fn = m_result_path;

        str_bow_fn += "bow_";
        str_bow_fn += str_hash_img;
        str_bow_fn += ".xml.gz";

        FileStorage  fs_bow( str_bow_fn, FileStorage::READ );

        if ( fs_bow.isOpened() )
        {
            fs_bow["mwh"] >> p_llc_data->m5_wh;
            fs_bow["mw"] >> p_llc_data->m5_w;
            fs_bow["mh"] >> p_llc_data->m5_h;

            vector<uchar>  llc_masks;
            fs_bow["llc_masks"] >> llc_masks;
            char  *p_data = (char*)(&llc_masks[0]);
            memmove((char*)(p_llc_data->llc_mask), p_data, MWH*MWH*sizeof(uchar));

            vector<float>  llc_idxs;
            fs_bow["llc_idxs"] >> llc_idxs;
            p_data = (char*)(&llc_idxs[0]);
            memmove((char*)(p_llc_data->llc_idx), p_data, MWH*MWH*KNN*sizeof(float));

            vector<float>  llc_weights;
            fs_bow["llc_weights"] >> llc_weights;
            p_data = (char*)(&llc_weights[0]);
            memmove((char*)(p_llc_data->llc_weight), p_data, MWH*MWH*KNN*sizeof(float));


            for (set<string>::iterator iter = class_names.begin(); iter != class_names.end(); iter++)
            {
                string     class_name = *iter;

                VecPoint   points;

                fs_bow[class_name] >> points;

                map_class_rois_points[class_name] = points;
            }
        }

        fs_bow.release();

        map<string, Mat>  map_class_hists02;
        map_class_hists02.clear();

        llc_data_to_class_hists(class_names, voc_matchs, p_llc_data, map_class_rois_points, map_class_hists02);

        double  distance_class = compare_class_hists(class_names, map_class_hists01, map_class_hists02);

        if (distance_class > 0.25)  map_class_distance_hashs.insert(multimap<double, string, greater<double> >::value_type(distance_class, str_hash_img));

        std::cout << str_path << "  distance_class :  " << distance_class << std::endl;
        //std::cout << str_path << "  distance_hists :  " << distance_hists << std::endl;

        delete   p_llc_data;
    }

    multimap<double, string, greater<double> >::iterator  iter01;

    for (iter01 = map_class_distance_hashs.begin(); iter01 != map_class_distance_hashs.end(); iter01++)
    {
        string  str_hash_img = iter01->second;

        _return.push_back(str_hash_img);
    }

    //event.SetString("........................................");
    //wxQueueEvent(&app, event.Clone());

    //wxThreadEvent  event_cls( wxEVT_THREAD, WORKER_CLS );
    //wxQueueEvent(&app, event_cls.Clone());
}

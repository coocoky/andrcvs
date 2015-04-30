#include "wx/wxprec.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include "wx/thread.h"
#include <wx/cmdline.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

#include "boost/thread.hpp"
#include "boost/asio.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/sha1.hpp>

#include "TranData.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TServerTransport.h>
#include <thrift/transport/TBufferTransports.h>

#include <thrift/protocol/TDebugProtocol.h>
#include <thrift/concurrency/ThreadManager.h>
//#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/concurrency/BoostThreadFactory.h>

#include "mcvfun.h"
#include "MyApp.h"

#include <stdio.h>
#include <conio.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::concurrency;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace std;
using namespace cv;
using namespace  ::cvrpc;
using boost::asio::ip::tcp;

// [vocabulary] [images_dir] [result_dir]
static const wxCmdLineEntryDesc cmdLineDesc[] =
{
    { wxCMD_LINE_SWITCH, "h", "help", "show this help message", wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
    { wxCMD_LINE_SWITCH,  NULL, "setup", "setup switch" },
    { wxCMD_LINE_SWITCH,  NULL, "run", "run server" },
    { wxCMD_LINE_OPTION,  NULL, "vocabulary",  "The file of vocabulary", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_OPTION,  NULL, "images",  "The directory of images", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_OPTION,  NULL, "result",  "The directory of result", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_OPTION,  NULL, "charset",  "Locale charset", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },

    { wxCMD_LINE_NONE }
};


void  local_ipv4(string  &str_json_ipv4)
{

    boost::asio::io_service io_service;
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(boost::asio::ip::host_name(),"");
    tcp::resolver::iterator it=resolver.resolve(query);

    string  str_ipv4;

    while(it!=tcp::resolver::iterator())
    {
        boost::asio::ip::address addr=(it++)->endpoint().address();
        if(addr.is_v6())
        {
            //std::cout<<"ipv6 address: ";
        }
        else
        {
            //std::cout<<"ipv4 address: ";

            str_ipv4 =  addr.to_string();
        }
        //std::cout<<addr.to_string()<<std::endl;
    }

    //std::cout<< str_ipv4 <<std::endl;

    boost::property_tree::ptree pt;
    pt.put("name", "cvrpc");
    pt.put("ipv4", str_ipv4);
    pt.put("port", RPC_PORT);

    std::stringstream s2;
    write_json(s2, pt);
    str_json_ipv4 = s2.str();
    //std::cout << str_json_ipv4 << std::endl;
}

void ser_udp()
{
    cout << "ser_udp" << endl;

    boost::asio::io_service io_service;
    boost::asio::ip::udp::socket udp_socket(io_service);
    boost::asio::ip::udp::endpoint local_add(boost::asio::ip::address::from_string("0.0.0.0"), UDP_PORT);

    udp_socket.open(local_add.protocol());
    udp_socket.bind(local_add);

    char receive_buffer[256];
    memset(receive_buffer, 0, 256);

    using namespace boost::property_tree;

    while (true)
    {
        boost::asio::ip::udp::endpoint send_point;
        udp_socket.receive_from(boost::asio::buffer(receive_buffer, 256), send_point);

        string  str_json_recv = receive_buffer;
        string  str_name;

        std::stringstream ss(str_json_recv);
        ptree pt;
        try
        {
            read_json(ss, pt);
            str_name = pt.get<string>("name");
        }
        catch(ptree_error & e)
        {
            cout << "json err " << endl;

            continue;
        }

        //cout << "recv:" << receive_buffer << endl;

        string str_txt = "recv: ";
        str_txt +=  receive_buffer;

        wxThreadEvent event( wxEVT_THREAD, WORKER_EVENT );

        event.SetString(str_txt);

        MyApp  &app = wxGetApp();
        wxQueueEvent(&app, event.Clone());


        if (str_name != "cvrpc")
        {
            cout << "json name err " << endl;
            continue;
        }

        memset(receive_buffer, 0, 256);

        string  str_json_ipv4;
        local_ipv4(str_json_ipv4);

        memmove(receive_buffer, str_json_ipv4.c_str(), str_json_ipv4.length());

        udp_socket.send_to(boost::asio::buffer(receive_buffer), send_point);
        //udp_socket.send_to(boost::asio::buffer(str_json_ipv4), send_point);
        memset(receive_buffer, 0, 256);
    }
}

void  getch_console()
{
    while (true)
    {
        if (getch() == 27)
        {
            wxThreadEvent event( wxEVT_THREAD, THREAD_QUIT );
            MyApp  &app = wxGetApp();
            wxQueueEvent(&app, event.Clone());
        }
    }
}

class TranDataHandler : virtual public TranDataIf
{
private:
    Mat                   main_hists;
    vector<string>        m_hashs;
    string                m_imgs_path;
    string                m_str_charset;
    string                sqlite_fn;
    string                m_result_path;

    map<string, string>   m_hash_fns;

    Mat   vocabulary;
    Mat   voc_matchs;

    Ptr<FeatureDetector>       detector;
    Ptr<DescriptorExtractor>   extractor;

    int                   id_imgs;

public:
    TranDataHandler(string voc_fn, string  imgs_path, string result_path, string str_charset)
    {
        // Your initialization goes here

        id_imgs = 1;

        std::string   vocabulary_file =  voc_fn;
        std::string   voc_matchs_fn = result_path + "/voc_matchs.xml.gz";

        m_result_path = result_path;

        sqlite_fn = result_path + "/imgs.db";

        m_imgs_path = imgs_path;
        m_str_charset = str_charset;

        FileStorage fs( vocabulary_file, FileStorage::READ );
        if ( fs.isOpened() )  fs["vocabulary"] >> vocabulary;

        std::cout << "vocabulary_file :  " << vocabulary_file << std::endl;
        std::cout << "vocabulary rows cols = " << vocabulary.rows << "  " << vocabulary.cols << std::endl;

        FileStorage fs_voc( voc_matchs_fn, FileStorage::READ );
        if ( fs_voc.isOpened() )  fs_voc["voc_matchs"] >> voc_matchs;

        std::cout << "voc_matchs rows cols = " << voc_matchs.rows << "  " << voc_matchs.cols << std::endl;

        //detector = FeatureDetector::create( "SURF" );
        extractor = DescriptorExtractor::create( "SURF" );

        //FileStorage fs_hists( hists_fn, FileStorage::READ );
        //if ( fs_hists.isOpened() )  fs_hists["surf_hists"] >> hists;

        //fstream      file_list;

        //file_list.open(imgs_fn.c_str(), ios::in);

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

            exit(-1);
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

    void hello_string(std::string& _return, const std::string& para)
    {
        // Your implementation goes here

        _return = "hello_string cvrpc para : ";

        _return += para;

        //printf("hello_string\n");

        std::cout << _return << std::endl;
    }

    void opencv_rpc(std::string& _return, const std::string& fun_name, const std::vector<std::string> & pa, const std::string& in_data)
    {

        // Your implementation goes here
        printf("opencv_rpc\n");
    }

    void read_image(std::string& _return, const std::string &file_hash, const std::map<std::string, std::string> &pa)
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

        MyApp  &app = wxGetApp();
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

    void image_match(std::vector<std::string> &_return, const std::string &img_data, const std::vector<std::string> &pa)
    {
        // Your implementation goes here
        //printf("image_match\n");

        std::cout << "image match ..." << std::endl;

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

        int  mns = 5;

        Mat  main_hist[mns];
        //Mat  main_hist02;

        Mat  main_masks[mns];
        //Mat  main_mask02;

        Mat  key_hists[mns];
        //Mat  hists02;

        LlcData  *p_llc_data01 = new LlcData;

        bool ret01 = true;
        int  ns=0;
        for (int ms=-4; ms<=4; ms+=2)
        {
            ret01 = voc_bow_mats(image01, vocabulary, voc_matchs, main_hist[ns], key_hists[ns], main_masks[ns], ms, p_llc_data01);
            ns++;
            if (ret01 == false) break;
        }

        mns = ns;
        //std::cout << "main_hist01 rows cols = " << main_hist01.rows << "  " << main_hist01.cols << std::endl;
        //std::cout << "main_hists rows cols = " << main_hists.rows << "  " << main_hists.cols << std::endl;

        delete  p_llc_data01;

        if (ret01 == false ) return;

        int     mknn = 30;
        //float   fbeta = 1e-4;

        vector<vector<DMatch> > matches[mns];

        Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce" );
        //Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "FlannBased" );

        for (int ns=0; ns<mns; ns++)
        {
            matcher -> knnMatch( main_hist[ns], main_hists, matches[ns], mknn );
        }

        multimap<double, string, greater<double> >  map_hists_bow_ns;

        vector<string>  hashs_knn[mns];

        for (int ns=0; ns<mns; ns++)
        {
            for (int n=0; n<mknn; n++)
            {
                vector<DMatch>  &matchesv = matches[ns][0];

                int       midx = matchesv[n].trainIdx;
                string    str_hash = m_hashs[midx];
                Mat       hist_db = main_hists.row(midx);

                double    distance = compareHist(main_hist[ns], hist_db, CV_COMP_CORREL);

                if (distance > 0.25)
                {
                    hashs_knn[ns].push_back(str_hash);
                }

            }
        }

        multimap<double, string, greater<double> >  map_hists_bows[mns];

        for (int ns=0; ns<mns; ns++)
        {
            sort_match_imgs(sqlite_fn, vocabulary, m_result_path, voc_matchs, main_hist[ns], key_hists[ns], hashs_knn[ns], map_hists_bows[ns]);
        }

        multimap<double, string, greater<double> >  map_hists_surf;
        double time_e = (double)getTickCount();
        double time_knn = (time_e - time_b)/getTickFrequency()*1000.0;

        char   chbuf[1024];
        snprintf(chbuf, 1000, "image match :  %f ms", time_knn);
        chbuf[1000] = '\0';

        wxThreadEvent event( wxEVT_THREAD, WORKER_EVENT );
        event.SetString(chbuf);

        MyApp  &app = wxGetApp();
        wxQueueEvent(&app, event.Clone());

        set<string>       fn_set;
        map<int, string>  fn_map;

        int  idx_map = 0;

        multimap<double, string, greater<double> >::iterator  iter;

        for (int ns=0; ns<mns; ns++)
        {
            for (iter = map_hists_bows[ns].begin(); iter != map_hists_bows[ns].end(); iter++)
            {
                double  distance_hists = iter->first;
                string  str_hash_img = iter->second;

                map_hists_bow_ns.insert(multimap<double, string, greater<double> >::value_type(distance_hists, str_hash_img));
            }
        }

        for (iter = map_hists_bow_ns.begin(); iter != map_hists_bow_ns.end(); iter++)
        {
            double  distance_hists = iter->first;
            string  str_hash_img = iter->second;

            char   chbuf[1024];
            string  str_fn = m_hash_fns[str_hash_img];
            snprintf(chbuf, 1000, "bow : %f  %s", distance_hists, str_fn.c_str());
            chbuf[1000] = '\0';

            if (distance_hists > 0.25 && fn_set.count(str_hash_img) == 0)
            {
                fn_set.insert(str_hash_img);
                fn_map[idx_map++] = str_hash_img;

                event.SetString(chbuf);
                wxQueueEvent(&app, event.Clone());
            }

            if (idx_map >= 3*8) break;
        }

        for (map<int, string>::iterator iter = fn_map.begin(); iter != fn_map.end(); iter++)
        {
            string  str_hash_img = iter->second;

            //string  str_utf8_fn;
            //cc.convert_string(str_fn, str_utf8_fn);

            _return.push_back(str_hash_img);
        }

        event.SetString("........................................");
        wxQueueEvent(&app, event.Clone());
    }
};

void help( const char* pro_name )
{
    std::cout << "OpenCV Thrift RPC Server..." << std::endl << std::endl;

    std::cout << "setup: " << pro_name << " setup [vocabulary file] [images dir] [result dir] " << std::endl;
    std::cout << "example: " << pro_name << " --setup  --vocabulary=./vocabulary.xml.gz   --images=../data/images01/  --result=../data/result01/" << std::endl;

    std::cout << std::endl;

    std::cout << "run: " << pro_name << " run [vocabulary file] [images dir] [result dir] " << std::endl;
    std::cout << "example: " << pro_name << " --run  --vocabulary=./vocabulary.xml.gz   --images=../data/images01/  --result=../data/result01/" << std::endl;
}

void  cvrpc_server(wxString voc_fn01, wxString  imgs_path01, wxString result_path01, wxString charset01)
{
    string   voc_fn;
    voc_fn = voc_fn01.mb_str();
    string   imgs_path;
    imgs_path = imgs_path01.mb_str();
    string   result_path;
    result_path = result_path01.mb_str();
    string   str_charset;
    str_charset = charset01.mb_str();

    shared_ptr<TranDataHandler> handler(new TranDataHandler(voc_fn, imgs_path, result_path, str_charset));
    shared_ptr<TProcessor> processor(new TranDataProcessor(handler));
    shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    shared_ptr<TServerTransport> serverTransport(new TServerSocket(RPC_PORT));

    shared_ptr<ThreadManager> threadManager = ThreadManager::newSimpleThreadManager(16);
    shared_ptr<BoostThreadFactory> threadFactory = shared_ptr<BoostThreadFactory> (new BoostThreadFactory());
    threadManager->threadFactory(threadFactory);
    threadManager->start();

    shared_ptr<TNonblockingServer> server(new TNonblockingServer(processor, protocolFactory, RPC_PORT, threadManager));
    server->serve();
}

void  setup(wxString voc_fn01, wxString  imgs_path01, wxString result_path01)
{
    string  voc_fn;
    voc_fn = voc_fn01.mb_str();
    string  imgs_path;
    imgs_path = imgs_path01.mb_str();
    string  result_path;
    result_path = result_path01.mb_str();

    string  sqlite_fn = result_path + "/imgs.db";
    string  voc_matchs_fn = result_path + "/voc_matchs.xml.gz";

    vector<string>   sub_dirs;
    vector<string>   file_paths;
    vector<string>   image_paths;
    vector<string>   imgs_list;

#ifdef _WIN32
    mkdir(result_path.c_str());
#else
    mkdir(result_path.c_str(), 0755);
#endif

    list_dirs(imgs_path, sub_dirs);

    int  size_dirs = sub_dirs.size();

    for (int i=0; i<size_dirs; i++)
    {
        cout<< sub_dirs[i] << endl;
    }

    cout << endl;

    create_imgs_db(sqlite_fn);

    std::string vocabularyFile =  voc_fn;

    Mat   vocabulary;

    FileStorage fs( vocabularyFile, FileStorage::READ );
    if ( fs.isOpened() )  fs["vocabulary"] >> vocabulary;

    std::cout << "vocabularyFile :  " << vocabularyFile << std::endl;
    std::cout << "vocabulary rows cols = " << vocabulary.rows << "  " << vocabulary.cols << std::endl;

    int  size_voc = SIZE_VOC;

    cout << "build vocabulary..." << endl;
    BOWKMeansTrainer bowTrainer( size_voc );
    Mat voc = bowTrainer.cluster( vocabulary );
    cout << "done build voc..." << endl;

    std::cout << "voc rows cols = " << voc.rows << "  " << voc.cols << std::endl;

    int     voc_knn = KNN;

    vector<vector<DMatch> > matches;

    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce" );
    //Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "FlannBased" );

    matcher -> knnMatch( voc, vocabulary, matches, voc_knn );

    Mat  voc_matchs = Mat::zeros(voc.rows, voc_knn, CV_32S);

    for (int n=0; n<voc.rows; n++)
    {
        vector<DMatch> &matchesv = matches[n];

        for (int i=0; i<voc_knn; i++)
        {
            voc_matchs.at<int>(n, i) = matchesv[i].trainIdx;
        }
    }

    FileStorage fs_voc( voc_matchs_fn, FileStorage::WRITE );
    if ( fs.isOpened() ) fs_voc << "voc_matchs" << voc_matchs;
    fs_voc.release();

    std::cout << "voc_matchs rows cols = " << voc_matchs.rows << "  " << voc_matchs.cols << std::endl;

    //Ptr<FeatureDetector> detector = FeatureDetector::create( "SURF" );
    Ptr<DescriptorExtractor> extractor = DescriptorExtractor::create( "SURF" );

    list_files(sub_dirs,  file_paths);
    list_images(file_paths,  image_paths);

    map<string, string>  hash_fn_map;
    set<string>   fn_set;

    read_db_map(sqlite_fn, hash_fn_map);

    map<string, string>::iterator  iter;

    for (iter = hash_fn_map.begin(); iter != hash_fn_map.end(); iter++)
    {
        string  str_fn = iter->second;

        fn_set.insert(str_fn);
    }

    int  size_images = image_paths.size();

    sqlite3_stmt *stmt;
    char ca[255];

    char *errmsg = 0;
    int   ret = 0;

    sqlite3 *db = 0;
    ret = sqlite3_open_v2(sqlite_fn.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    if(ret != SQLITE_OK)
    {
        fprintf(stderr,"Cannot open db: %s\n", sqlite3_errmsg(db));
        return;
    }

    printf("Open database\n");

    vector<float>  hist_data;

    LlcData *p_llc_data = new LlcData;

    Mat  hist = Mat::zeros(1, SIZE_VOC, CV_32F);

    for (int i=0; i<size_images; i++)
    {
        string  str_path = image_paths[i];
        string  str_fn = str_path.substr(imgs_path.length());

        std::cout << "str_fn : " << str_fn << std::endl;

        if (fn_set.count(str_fn) > 0)  continue;

        struct stat  fi_stat;

        stat(str_path.c_str(), &fi_stat);

        fstream  fs_img(str_path.c_str(), ios_base::binary|ios_base::in);

        vector<uchar>  buff;

        buff.resize(fi_stat.st_size);

        char  *p_buf = (char*)&(buff[0]);

        fs_img.read(p_buf, fi_stat.st_size);

        boost::uuids::detail::sha1   sha;
        sha.process_bytes(&buff[0], fi_stat.st_size);

        uint32_t digest[5];
        sha.get_digest(digest);

        char    ch_uuid[100];
        sprintf(ch_uuid, "%08x%08x%08x%08x%08x", digest[0], digest[1], digest[2], digest[3], digest[4]);
        string  str_hash = ch_uuid;

        if (hash_fn_map.find(str_hash) != hash_fn_map.end()) continue;

        hash_fn_map[str_hash] = str_fn;

        Mat image = imdecode(buff, CV_LOAD_IMAGE_GRAYSCALE);

        if (image.data == 0 || image.cols < MIN_IMG_WH || image.rows < MIN_IMG_WH) continue;

        Mat  main_hist;
        Mat  main_mask;
        Mat  key_hists;
        Mat  hists;

        memset(p_llc_data, 0, sizeof(LlcData));

        bool ret01 = voc_bow_mats(image, vocabulary, voc_matchs, main_hist, key_hists, main_mask, 0, p_llc_data);

        img_to_db(db, str_hash, str_fn, result_path, main_hist, main_mask, p_llc_data);
    }

    delete  p_llc_data;

    //sqlite3_finalize(stmt);
    sqlite3_free(errmsg);
    sqlite3_close(db);

    printf("Close database\n");
}

int main(int argc, char **argv)
{
    cv::initModule_nonfree();

    wxApp::CheckBuildOptions(WX_BUILD_OPTIONS_SIGNATURE, "program");

    wxInitializer initializer;

    if ( !initializer )
    {
        fprintf(stderr, "Failed to initialize the wxWidgets library, aborting.");
        return -1;
    }

#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested = MAKEWORD( 2, 2 );
    err = WSAStartup( wVersionRequested, &wsaData );
    if ( err )
    {
        std::cout << "winsock error !!" << std::endl;
        exit(-1);
    }
#endif

    if (argc == 1)
    {
        help(argv[0]);
        return -1;
    }

    wxString  wstr_cml01 = wxEmptyString;
    wxString  wstr_cml02 = wxEmptyString;
    wxString  wstr_cml03 = wxEmptyString;
    wxString  wstr_cml04 = wxEmptyString;

    wxCmdLineParser parser(cmdLineDesc, argc, argv);

    if ( parser.Parse() == 0)
    {
        if (parser.Found("setup") && parser.Found("vocabulary", &wstr_cml01)
                &&  parser.Found("images", &wstr_cml02) &&  parser.Found("result", &wstr_cml03))
        {
            setup(wstr_cml01, wstr_cml02, wstr_cml03);
        }

        if (parser.Found("run") && parser.Found("vocabulary", &wstr_cml01)
                &&  parser.Found("images", &wstr_cml02) &&  parser.Found("result", &wstr_cml03))
        {
            if ( parser.Found("charset", &wstr_cml04) == false )
            {
                wstr_cml04 = "utf-8";
            }

            boost::thread  thrd_udp(ser_udp);
            boost::thread  thrd_con(getch_console);
            boost::thread  thrd_cvrpc(cvrpc_server, wstr_cml01, wstr_cml02, wstr_cml03, wstr_cml04);

            MyApp  *myapp = new MyApp();

            myapp->OnInit();
            myapp->MainLoop();
        }
    }

    return 0;
}

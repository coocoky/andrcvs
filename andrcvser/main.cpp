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
    Mat                   hists;
    vector<string>        paths;
    string                m_imgs_path;
    string                m_str_charset;

    Mat   vocabulary;
    Mat   voc_matchs;

    Ptr<FeatureDetector>       detector;
    Ptr<DescriptorExtractor>   extractor;

public:
    TranDataHandler(string voc_fn, string  imgs_path, string result_path, string str_charset)
    {
        // Your initialization goes here

        std::string   vocabulary_file =  voc_fn;
        std::string   voc_matchs_fn = result_path + "/voc_matchs.xml.gz";
        std::string   sqlite_fn = result_path + "/imgs.db";

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

        int result = sqlite3_open_v2(sqlite_fn.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

        if (SQLITE_OK != result)
        {
            std::cout << "sqlite3_open error !!" << std::endl;

            exit(-1);
        }

        result = sqlite3_prepare(db,"SELECT * FROM imgs;", -1, &stmt, &zTail);
        //result = sqlite3_prepare(db,"SELECT * FROM imgs WHERE id=3;", -1, &stmt, &zTail);
        //result = sqlite3_prepare(db,"SELECT id, value, weight FROM test;", -1, &stmt, &zTail);

        result = sqlite3_step(stmt);

        while( result == SQLITE_ROW )
        {
            int id = sqlite3_column_int( stmt, 0 );
            const char* path = (char*)sqlite3_column_text( stmt, 1 );

            const void   *p_buf = sqlite3_column_blob(stmt, 2);
            int  size_buf = sqlite3_column_bytes(stmt, 2);

            //Mat hist = Mat::zeros(1, VOCA_COLS, CV_32F);
            Mat hist = Mat::zeros(1, SIZE_VOC, CV_32F);

            vector<float>   hist_data;
            hist_data.resize(hist.cols);

            char  *p_data = (char*)(&hist_data[0]);

            memmove(p_data,  p_buf, size_buf);

            for (int i=0; i<hist.cols; i++)
            {
                hist.at<float>(0, i) = hist_data[i];
            }

            //std::cout << "id : " << id << "  path : " << path << " hist.rows :" << hist.cols << std::endl;

            paths.push_back(path);
            hists.push_back(hist);

            result = sqlite3_step(stmt);
        }

        sqlite3_finalize(stmt);
        sqlite3_close(db);

        std::cout << "hists rows cols = " << hists.rows << "  " << hists.cols << std::endl;
    }

    void hello_string(std::string& _return, const std::string& para)
    {
        // Your implementation goes here

        _return = "hello_string cvrpc para : ";

        _return += para;

        //printf("hello_string\n");

        std::cout << _return << std::endl;
    }

    void read_data(std::string& _return, const std::string& name, const int32_t length)
    {
        // Your implementation goes here

        printf("read_data\n");
    }

    void opencv_rpc(std::string& _return, const std::string& fun_name, const std::vector<std::string> & pa, const std::string& in_data)
    {

        // Your implementation goes here
        printf("opencv_rpc\n");
    }

    void read_image(std::string& _return, const std::string& file_name, const std::map<std::string, std::string> & pa)
    {
        // Your implementation goes here
        //printf("read_image\n");

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
        wxQueueEvent(&app, event.Clone());

        if (!img_src.data)
        {
            _return.clear();
            return;
        }

        vector<uchar>   buf_img;
        vector<int>     params;

        buf_img.clear();
        params.push_back( cv::IMWRITE_JPEG_QUALITY );
        //params.push_back( 75 );
        params.push_back( 90 );

        imencode( ".jpg", img_src, buf_img, params );

        int  size_buf = buf_img.size();
        char   *p_ch = (char*)(&buf_img[0]);

        _return.assign(p_ch, size_buf);
    }

    void image_match(std::vector<std::string> & _return, const std::string& fun_name, const std::string& img_data, const std::vector<std::string> & pa)
    {
        // Your implementation goes here
        //printf("image_match\n");

        vector<uchar>   buf_img;

        int  size_str = img_data.size();

        buf_img.resize(size_str);

        char     *p_buf = (char*)(&buf_img[0]);
        char     *p_str = (char*)img_data.c_str();

        memmove(p_buf, p_str, size_str);

        Mat img_dst, img_gray, detected_edges;

        Mat image01 = imdecode(buf_img, CV_LOAD_IMAGE_GRAYSCALE);
        //Mat image01 = imdecode(buf_img, CV_LOAD_IMAGE_COLOR);

        //Mat image_120_01; //= Mat::zeros( 120, 120, CV_8UC1);

        //resize(image01, image_120_01, Size(120, 120));

        //Mat  hist01 = surf_hist_120(image_120_01, extractor, 0.0, 30);

        //Mat  hist01 = Mat::zeros(1, VOCA_COLS, CV_32F);
        Mat  hist01 = Mat::zeros(1, SIZE_VOC, CV_32F);

        //bool ret = calc_hist(image01, vocabulary, detector, hist01);
        bool ret = voc_hist(image01, vocabulary, voc_matchs, hist01);

        if (ret == false) return;

        int     mknn = 9;
        //float   fbeta = 1e-4;

        vector<vector<DMatch> > matches;

        Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce" );
        //Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "FlannBased" );

        matcher -> knnMatch( hist01, hists, matches, mknn );

        CodeConverter  cc(m_str_charset.c_str(), "utf-8");

        for (int n=0; n<mknn; n++)
        {
            vector<DMatch> &matchesv = matches[0];

            string   str_fn = paths[matchesv[n].trainIdx];

            Mat  hist02 = hists.row(matchesv[n].trainIdx);

            double distance = compareHist(hist01, hist02, CV_COMP_CORREL);

            string   str_utf8_fn = "";

            if (m_str_charset == "utf-8")
            {
                str_utf8_fn = str_fn;
            }
            else
            {
                cc.convert_string(str_fn, str_utf8_fn);
            }

            //if (distance > 0.80) _return.push_back(str_fn);

            //int cols = image01.cols;
            //int rows = image01.rows;

            //std::cout << "image match :  " << image01.cols << "  " << image01.rows << "  " << distance << std::endl;

            char   chbuf[1024];

            snprintf(chbuf, 1000, "image match :  %d  %d  %f", image01.cols, image01.rows, distance);

            chbuf[1000] = '\0';

            wxThreadEvent event( wxEVT_THREAD, WORKER_EVENT );

            event.SetString(chbuf);

            MyApp  &app = wxGetApp();
            wxQueueEvent(&app, event.Clone());

            _return.push_back(str_utf8_fn);
        }
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

/*
wxThread::ExitCode MyThread::Entry()
{
    shared_ptr<TranDataHandler> handler(new TranDataHandler(voc_fn, imgs_path, result_path));
    shared_ptr<TProcessor> processor(new TranDataProcessor(handler));
    shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    shared_ptr<TServerTransport> serverTransport(new TServerSocket(RPC_PORT));


    shared_ptr<ThreadManager> threadManager = ThreadManager::newSimpleThreadManager(16);
    //shared_ptr<PosixThreadFactory> threadFactory = shared_ptr<PosixThreadFactory> (new PosixThreadFactory());
    shared_ptr<BoostThreadFactory> threadFactory = shared_ptr<BoostThreadFactory> (new BoostThreadFactory());
    threadManager->threadFactory(threadFactory);
    threadManager->start();

    shared_ptr<TNonblockingServer> server(new TNonblockingServer(processor, protocolFactory, RPC_PORT, threadManager));
    server->serve();
}
*/

void  cvrpc_server(wxString voc_fn01, wxString  imgs_path01, wxString result_path01, wxString charset01)
{
    string   voc_fn; voc_fn = voc_fn01.mb_str();
    string   imgs_path; imgs_path = imgs_path01.mb_str();
    string   result_path; result_path = result_path01.mb_str();
    string   str_charset; str_charset = charset01.mb_str();

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
    string  voc_fn; voc_fn = voc_fn01.mb_str();
    string  imgs_path; imgs_path = imgs_path01.mb_str();
    string  result_path; result_path = result_path01.mb_str();

    string  sqlite_fn = result_path + "/imgs.db";
    string  voc_matchs_fn = result_path + "/voc_matchs.xml.gz";

    //fstream          fs_list;
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

    int     knn = KNN;
    float   fbeta = 1e-4;

    vector<vector<DMatch> > matches;

    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce" );
    //Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "FlannBased" );

    matcher -> knnMatch( voc, vocabulary, matches, knn );

    Mat  voc_matchs = Mat::zeros(voc.rows, knn, CV_32S);

    //Mat  hist = Mat::zeros(1, size_voc, CV_32F);

    for (int n=0; n<voc.rows; n++)
    {
        vector<DMatch> &matchesv1 = matches[n];

        for (int i=0; i<knn; i++)
        {
            int  idx01 = matchesv1[i].trainIdx;

            //hist.at<float>(0, n) += sum_query_descriptor01.at<float>(0, idx01);

            voc_matchs.at<int>(n, i) = matchesv1[i].trainIdx;

            //std::cout << idx01 << ", ";
        }

        //std::cout << std::endl;
    }

    FileStorage fs_voc( voc_matchs_fn, FileStorage::WRITE );
    if ( fs.isOpened() ) fs_voc << "voc_matchs" << voc_matchs;

    std::cout << "voc_matchs rows cols = " << voc_matchs.rows << "  " << voc_matchs.cols << std::endl;

    //Ptr<FeatureDetector> detector = FeatureDetector::create( "SURF" );
    Ptr<DescriptorExtractor> extractor = DescriptorExtractor::create( "SURF" );

    list_files(sub_dirs,  file_paths);
    list_images(file_paths,  image_paths);

    //fs_list.open(imgs_fn.c_str(), ios::out);

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

    Mat  hist = Mat::zeros(1, VOCA_COLS, CV_32F);

    for (int i=0; i<size_images; i++)
    {
        string  str_path = image_paths[i];
        string  str_fn = str_path.substr(imgs_path.length());

        std::cout << "str_fn : " << str_fn << std::endl;

        Mat image = imread(str_path,  CV_LOAD_IMAGE_GRAYSCALE);
        //Mat image = imread(str_path,  CV_LOAD_IMAGE_COLOR);

        //if (image.data == 0 ) continue;
        if (image.data == 0 || image.cols < MIN_IMG_WH || image.rows < MIN_IMG_WH) continue;

        //fs_list << str_fn << endl;

        //Mat image_120; //= Mat::zeros( 120, 120, CV_8UC1);
        //resize(image01, image_120, Size(120, 120));
        //Mat  hist01 = surf_hist_120(image_120, extractor, 0.0, 30);

        //hist = Mat::zeros(1, VOCA_COLS, CV_32F);
        //bool ret = calc_hist(image, vocabulary, detector, hist);

        hist = Mat::zeros(1, SIZE_VOC, CV_32F);
        bool ret = voc_hist(image, vocabulary, voc_matchs, hist);

        if (ret == false) continue;

        //descriptors.push_back(hist);

        sqlite3_prepare_v2(db,"insert into imgs(id, path, hist) values(null,?,?)", -1, &stmt, 0);

        sqlite3_bind_text(stmt, 1, str_fn.c_str(), str_fn.length() , NULL);

        hist_data.clear();
        hist_data.resize(hist.cols);

        for (int i=0; i<hist.cols; i++)
        {
            hist_data[i] = hist.at<float>(0, i);
        }

        char   *p_buf = (char*)(&hist_data[0]);

        sqlite3_bind_blob(stmt, 2, p_buf, hist.cols*sizeof(float), NULL);

        sqlite3_step(stmt);

        //if (ret != SQLITE_OK) fprintf(stderr,"db: %s\n",sqlite3_errmsg(db));

        sqlite3_reset(stmt);
    }

    //FileStorage fs_hists( hists_fn, FileStorage::WRITE );
    //if ( fs_hists.isOpened() )  fs_hists << "surf_hists" << descriptors;

    sqlite3_finalize(stmt);

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

    if (argc == 1) { help(argv[0]); return -1; }

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
            //std::cout << str_cml01 << std::endl;
            //std::cout << str_cml02 << std::endl;
            //std::cout << str_cml03 << std::endl;

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

            //MyThread  *p_thread = new MyThread(wstr_cml01, wstr_cml02, wstr_cml03);

            MyApp  *myapp = new MyApp();

            myapp->OnInit();
            myapp->MainLoop();
        }
    }

    return 0;
}

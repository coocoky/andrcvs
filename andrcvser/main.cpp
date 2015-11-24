#include <cstdio>
#include <cstdlib>
//#include <conio.h>

#include "wx/wxprec.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/sha1.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TServerTransport.h>
#include <thrift/transport/TBufferTransports.h>

#include <thrift/protocol/TDebugProtocol.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/concurrency/BoostThreadFactory.h>

#include "mcvfun.h"
#include "MyApp.h"
#include "TranDataHandler.h"

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

        //erase();

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

/*
int  minit_curses()
{
    initscr();
    curs_set(0);
    if(start_color() == OK)
    {
        init_pair(1, COLOR_WHITE, COLOR_BLUE);
        attron(COLOR_PAIR(1));
    }
    wbkgd(stdscr, COLOR_PAIR(1));
}

void  getch_console()
{
    while (true)
    {
        usleep(1000);

        if (getch() == 27)
        {
            wxThreadEvent event( wxEVT_THREAD, THREAD_QUIT );
            MyApp  &app = wxGetApp();
            wxQueueEvent(&app, event.Clone());
        }
    }
}
*/

void help( const char* pro_name )
{
    std::cout << "OpenCV Thrift RPC Server..." << std::endl << std::endl;

    std::cout << "setup: " << pro_name << " setup [vocabulary file] [images dir] [result dir] " << std::endl;
    std::cout << "example: " << pro_name << " --setup  --vocabulary=./vocabulary.xml.gz   --images=../data/images01/  --result=../data/result01/" << std::endl;

    std::cout << std::endl;

    std::cout << "run: " << pro_name << " run [vocabulary file] [images dir] [result dir] " << std::endl;
    std::cout << "example: " << pro_name << " --run  --vocabulary=./vocabulary.xml.gz   --images=../data/images01/  --result=../data/result01/" << std::endl;
}

void  cvrpc_server(wxString voc_fn01, wxString  imgs_path01, wxString result_path01)
{
    string   voc_fn;
    voc_fn = voc_fn01.mb_str();
    string   imgs_path;
    imgs_path = imgs_path01.mb_str();
    string   result_path;
    result_path = result_path01.mb_str();
    //string   str_charset;
    //str_charset = charset01.mb_str();

    boost::shared_ptr<TranDataHandler> handler(new TranDataHandler(voc_fn, imgs_path, result_path));
    boost::shared_ptr<TProcessor> processor(new TranDataProcessor(handler));
    boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(RPC_PORT));

    boost::shared_ptr<ThreadManager> threadManager = ThreadManager::newSimpleThreadManager(16);
    boost::shared_ptr<BoostThreadFactory> threadFactory = boost::shared_ptr<BoostThreadFactory> (new BoostThreadFactory());
    threadManager->threadFactory(threadFactory);
    threadManager->start();

    boost::shared_ptr<TNonblockingServer> server(new TNonblockingServer(processor, protocolFactory, RPC_PORT, threadManager));
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

    vector<string>    strs_rois_class;
    set<string>       class_names;
    Mat               class_hists;

    map<string, Mat>  map_class_vocs;

    int  pos = voc_fn.rfind("/");

    string  str_class_dir01 = voc_fn.substr(0, pos);
    string  str_class_dir02 = voc_fn.substr(0, pos);

    str_class_dir01 += "/class_240";
    str_class_dir02 += "/class_320";

    std::cout << "str_class_dir01 : " << str_class_dir01 << std::endl;
    std::cout << "str_class_dir02 : " << str_class_dir02 << std::endl;

    read_class_db(str_class_dir01, class_names, map_class_vocs, strs_rois_class, class_hists);
    read_class_db(str_class_dir02, class_names, map_class_vocs, strs_rois_class, class_hists);

    std::cout << "strs_rois_class.size : " << strs_rois_class.size() << std::endl;
    std::cout << "class_hists.rows : " << class_hists.rows << std::endl;

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
    if ( fs_voc.isOpened() ) fs_voc << "voc_matchs" << voc_matchs;
    fs_voc.release();

    std::cout << "voc_matchs rows cols = " << voc_matchs.rows << "  " << voc_matchs.cols << std::endl;

    //Ptr<EXTRACTOR> extractor = EXTRACTOR::create(RADIUS);

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
        //return;
    }

    //printf("Open database\n");

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

        //bool ret01 = voc_bow_mats(image, vocabulary, voc_matchs, main_hist, key_hists, main_mask, 1.0, p_llc_data);

        map<string, VecPoint >   map_class_rois_points;
        map_class_rois_points.clear();

        img_zoom_class_rois(image, vocabulary, voc_matchs, ZOOM_SIZE, class_names, strs_rois_class, class_hists,
                                main_hist, p_llc_data, map_class_rois_points);

        img_to_db(db, str_hash, str_fn, result_path, main_hist, main_mask, p_llc_data);

        //map<string, RoiCode>  map_class_roicode;
        //map_class_roicode.clear();

        //class_hists_codec(class_names, class_hists, map_class_hists, map_class_roicode);

        //class_bow_save(str_hash, result_path, class_names, map_class_roicode);

        class_bow_save(str_hash, result_path, class_names, p_llc_data, map_class_rois_points);
    }

    delete  p_llc_data;

    //sqlite3_finalize(stmt);
    sqlite3_free(errmsg);
    sqlite3_close(db);

    printf("Close database\n");
}

int main(int argc, char **argv)
{
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
        //std::cout << "wstr_cml01 : " << wstr_cml01 << std::endl;


        if (parser.Found("setup") && parser.Found("vocabulary", &wstr_cml01)
                &&  parser.Found("images", &wstr_cml02) &&  parser.Found("result", &wstr_cml03))
        {
            setup(wstr_cml01, wstr_cml02, wstr_cml03);
        }


        if (parser.Found("run") && parser.Found("vocabulary", &wstr_cml01)
                &&  parser.Found("images", &wstr_cml02) &&  parser.Found("result", &wstr_cml03))
        {

            std::cout << "wstr_cml01 : " << wstr_cml01 << std::endl;
            std::cout << "wstr_cml02 : " << wstr_cml02 << std::endl;
            std::cout << "wstr_cml03 : " << wstr_cml03 << std::endl;
            std::cout << "wstr_cml04 : " << wstr_cml04 << std::endl;

            if ( parser.Found("charset", &wstr_cml04) == false )
            {
                wstr_cml04 = "utf-8";
            }


            boost::thread  thrd_udp(ser_udp);
            //boost::thread  thrd_con(getch_console);
            boost::thread  thrd_cvrpc(cvrpc_server, wstr_cml01, wstr_cml02, wstr_cml03);

            //minit_curses();

            //exit(0);

            MyApp  *myapp = new MyApp();

            myapp->OnInit();
            myapp->MainLoop();

        }
    }

    return 0;
}

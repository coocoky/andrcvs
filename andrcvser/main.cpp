#include <opencv2/opencv.hpp>
#include <opencv2/nonfree/nonfree.hpp>

#include <iostream>

#include "boost/thread.hpp"
#include "boost/asio.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "TranData.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include <mcvfun.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace std;
using namespace cv;
using namespace  ::cvrpc;
using boost::asio::ip::tcp;

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

        cout << "recv:" << receive_buffer << endl;
        //cout << "str_name:" << str_name << endl;

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

class TranDataHandler : virtual public TranDataIf
{
private:
    Mat                   hists;
    vector<string>        paths;
    string                m_imgs_path;

    Mat  vocabulary;

    Ptr<FeatureDetector>       detector;
    Ptr<DescriptorExtractor>   extractor;

public:
    TranDataHandler(string voc_fn, string  imgs_path, string hists_fn, string  imgs_fn)
    {
        // Your initialization goes here

        std::string vocabularyFile =  voc_fn;

        m_imgs_path = imgs_path;

        FileStorage fs( vocabularyFile, FileStorage::READ );
        if ( fs.isOpened() )  fs["vocabulary"] >> vocabulary;

        std::cout << "vocabularyFile :  " << vocabularyFile << std::endl;
        std::cout << "vocabulary rows cols = " << vocabulary.rows << "  " << vocabulary.cols << std::endl;

        detector = FeatureDetector::create( "SURF" );
        extractor = DescriptorExtractor::create( "SURF" );

        FileStorage fs_hists( hists_fn, FileStorage::READ );
        if ( fs_hists.isOpened() )  fs_hists["surf_hists"] >> hists;

        std::cout << "hists rows cols = " << hists.rows << "  " << hists.cols << std::endl;

        fstream      file_list;

        file_list.open(imgs_fn.c_str(), ios::in);

        string        img_fn;
        string        img_path;

        while (true)
        {
            file_list >> img_fn ;
            if (file_list.good() == false ) break;
            //img_path = imgs_path + img_fn;
            paths.push_back(img_fn);
        }
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

        CodeConverter  cc("utf-8", char_lang);

        string  str_fn = "";

        cc.convert_string(file_name, str_fn);

        string  str_path = m_imgs_path + str_fn;

        Mat  img_src = imread(str_path, CV_LOAD_IMAGE_COLOR);

        //string  str_txt = str_fn.substr(m_imgs_path.length());
        std::cout << "read image :  " << img_src.cols << "  " << img_src.rows << "  " << str_fn << std::endl;

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

        Mat  hist01 = Mat::zeros(1, VOCA_COLS, CV_32F);

        calc_hist(image01, vocabulary, detector, hist01);

        int     mknn = 9;
        //float   fbeta = 1e-4;

        vector<vector<DMatch> > matches;

        Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce" );
        //Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "FlannBased" );

        matcher -> knnMatch( hist01, hists, matches, mknn );

        CodeConverter  cc(char_lang, "utf-8");

        for (int n=0; n<mknn; n++)
        {
            vector<DMatch> &matchesv = matches[0];

            string   str_fn = paths[matchesv[n].trainIdx];

            Mat  hist02 = hists.row(matchesv[n].trainIdx);

            double distance = compareHist(hist01, hist02, CV_COMP_CORREL);

            string   str_utf8_fn = "";

            cc.convert_string(str_fn, str_utf8_fn);

            //if (distance > 0.80) _return.push_back(str_fn);

            //int cols = image01.cols;
            //int rows = image01.rows;

            std::cout << "image match :  " << image01.cols << "  " << image01.rows << "  " << distance << std::endl;

            _return.push_back(str_utf8_fn);
        }
    }
};

void help( const char* pro_name )
{
    std::cout << "OpenCV Thrift RPC Server..." << std::endl << std::endl;

    std::cout << "setup: " << pro_name << " setup [vocabulary] [images_dir] [result_dir] " << std::endl;
    std::cout << "example: " << pro_name << " setup  ./vocabulary_surf.xml.gz  ../data/images01/  ../data/result/" << std::endl;

    std::cout << std::endl;

    std::cout << "run: " << pro_name << " run [vocabulary] [images_dir] [result_dir] " << std::endl;
    std::cout << "example: " << pro_name << " run  ./vocabulary_surf.xml.gz  ../data/images01/  ../data/result/" << std::endl;
}

void  setup(char* argv[])
{
    string  voc_fn = argv[2];
    string  imgs_path = argv[3];
    string  result_path = argv[4];

    string  hists_fn = result_path + "/hists.xml.gz";
    string  imgs_fn = result_path + "/images.list";

    fstream          fs_list;
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

    std::string vocabularyFile =  voc_fn;

    Mat  vocabulary;

    FileStorage fs( vocabularyFile, FileStorage::READ );
    if ( fs.isOpened() )  fs["vocabulary"] >> vocabulary;

    std::cout << "vocabularyFile :  " << vocabularyFile << std::endl;
    std::cout << "vocabulary rows cols = " << vocabulary.rows << "  " << vocabulary.cols << std::endl;

    Ptr<FeatureDetector> detector = FeatureDetector::create( "SURF" );
    Ptr<DescriptorExtractor> extractor = DescriptorExtractor::create( "SURF" );

    list_files(sub_dirs,  file_paths);
    list_images(file_paths,  image_paths);

    fs_list.open(imgs_fn.c_str(), ios::out);

    int  size_images = image_paths.size();

    Mat  descriptors;

    for (int i=0; i<size_images; i++)
    {
        string  str_path = image_paths[i];
        string  str_fn = str_path.substr(imgs_path.length());

        std::cout << "str_fn : " << str_fn << std::endl;

        Mat image = imread(str_path,  CV_LOAD_IMAGE_GRAYSCALE);
        //Mat image = imread(str_path,  CV_LOAD_IMAGE_COLOR);

        if (image.data == 0 || image.cols < MIN_IMG_WH || image.rows < MIN_IMG_WH) continue;

        fs_list << str_fn << endl;

        //Mat image_120; //= Mat::zeros( 120, 120, CV_8UC1);
        //resize(image01, image_120, Size(120, 120));
        //Mat  hist01 = surf_hist_120(image_120, extractor, 0.0, 30);

        Mat hist = Mat::zeros(1, VOCA_COLS, CV_32F);
        calc_hist(image, vocabulary, detector, hist);
        descriptors.push_back(hist);
    }

    FileStorage fs_hists( hists_fn, FileStorage::WRITE );
    if ( fs_hists.isOpened() )  fs_hists << "surf_hists" << descriptors;
}

void  run_ser(char* argv[])
{
#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested =MAKEWORD( 2, 2 );
    err = WSAStartup( wVersionRequested, &wsaData );
    if ( err ) { std::cout << "winsock error !!" << std::endl; exit(-1); }
#endif

    string  voc_fn = argv[2];
    string  imgs_path = argv[3];
    string  result_path = argv[4];

    string  hists_fn = result_path + "/hists.xml.gz";
    string  imgs_fn = result_path + "/images.list";

    boost::thread thrd(&ser_udp);

    shared_ptr<TranDataHandler> handler(new TranDataHandler(voc_fn, imgs_path, hists_fn, imgs_fn));
    shared_ptr<TProcessor> processor(new TranDataProcessor(handler));
    shared_ptr<TServerTransport> serverTransport(new TServerSocket(RPC_PORT));
    shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
    server.serve();
}

int main(int argc, char **argv)
{
    cv::initModule_nonfree();

    if ( argc < 2)
    {
        help( argv[0] );
        return -1;
    }

    string  str_cmd = argv[1];

    if (str_cmd == "setup")
    {
        setup(argv);
        return 0;
    }

    if (str_cmd == "run")
    {
        run_ser(argv);
        return 0;
    }

    return 0;
}

#ifndef TRANDATAHANDLER_H
#define TRANDATAHANDLER_H

#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d.hpp>

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

#include "TranData.h"
#include <thrift/protocol/TBinaryProtocol.h>

using namespace ::apache::thrift;

using boost::shared_ptr;

using namespace std;
using namespace cv;
using namespace  ::cvrpc;
using boost::asio::ip::tcp;

class TranDataHandler : virtual public TranDataIf
{
public:
    TranDataHandler(string voc_fn, string  imgs_path, string result_path);

private:
    void image_match(std::vector<std::string> &_return, const std::string &img_data, const std::vector<std::string> &pa);
    void read_image(std::string& _return, const std::string &file_hash, const std::map<std::string, std::string> &pa);
    void opencv_rpc(std::string& _return, const std::string& fun_name, const std::vector<std::string> & pa, const std::string& in_data);
    void hello_string(std::string& _return, const std::string& para);

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

    vector<string>       strs_rois_class;
    set<string>          class_names;
    map<string, Mat>     map_class_vocs;
    Mat                  class_hists;

    int               id_imgs;
};

#endif // TRANDATAHANDLER_H

namespace cpp  cvrpc
namespace java cvrpc

service  TranData{ 
  string hello_string(1:string para) 
  binary opencv_rpc(1:string fun_name, 2:list<string> pa, 3:binary in_data)
  binary read_image(1:string file_name, 2:map<string,string> pa)
  list<string> image_match(1:binary img_data, 2:list<string> pa)
 } 

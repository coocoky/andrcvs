                     android thrift rpc opencv servers 

                     
  andrcvs android  client send a image to andrcvs server, server return N like images.

  compile:

     andrcvs android client: eclipse android adt open andrcvcli/andrcvs project.

     andrcvs server: Win32/MinGW CodeBlocks open andrcvser andrcvser.cbp. :-)

       server requirements: opencv  3.0  and  opencv_contrib
                            boost   1.59
                            thrift  0.92 
                            wxWidgets 3.0
                            sqlite    3.8
                            
       opencv_contrib  https://github.com/Itseez/opencv_contrib
       
       recommend  msys2:  https://github.com/msys2/msys2.github.io

  simple use:

     1. cmds dir run "setuprcv01.cmd".  run server setup cmd scan images save image data
                                        to sqlite db. 

     2. cmds dir run "runrcv01.cmd".    OpenCV bow knn image match thrift rpc server.

     3. run  andrcvs android client. 

                                                               zhujiang

                                                      email: zhujiangmail@hotmail.com

                                                                          2015.11.17

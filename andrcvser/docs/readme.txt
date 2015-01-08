                     android thrift rpc opencv servers 


  andrcvs android  client  send  a image to andrcvs server, server return n like images.

  compile:

     andrcvs android  client: eclipse android adt open andrcvcli/andrcvs project.

     andrcvs server: Win32/MinGW CodeBlocks open andrcvser andrcvser.cbp.

       server requirements: opencv  2.4.x
                            boost   1.55
                            thrift  0.92 
                            wxWidgets  3.0

  simple use:

     1. cmds dir run "setuprcv01.cmd".  run server setup cmd scan images save image data to sqlite db. 

     2. cmds dir run "runrcv01.cmd".    OpenCV bow knn image match thrift rpc server.

     3. run  andrcvs android client. 

                                                                                        zhujiang

                                                                        email: zhujiangmail@hotmail.com

                                                                               2015.01.08


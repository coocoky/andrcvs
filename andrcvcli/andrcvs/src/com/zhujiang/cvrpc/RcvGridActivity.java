package com.zhujiang.cvrpc;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.CalendarView;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.Toast;

import org.apache.thrift.TApplicationException;
import org.json.JSONException;
import org.json.JSONObject;
import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.android.Utils;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.Scalar;
import org.opencv.core.Size;
import org.opencv.imgproc.Imgproc;
import org.opencv.core.MatOfByte;
import org.opencv.core.MatOfInt;
import org.opencv.highgui.Highgui;

import org.apache.thrift.TException;
import org.apache.thrift.transport.TTransport;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.opencv.ml.CvANN_MLP;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import cvrpc.*;

import static com.zhujiang.cvrpc.RcvGridAdapter.*;

public class RcvGridActivity extends Activity {

    private Mat  mat_show;
    private Mat  mat_resize;
    private Mat  mat_rpc;
    private RcvGridAdapter rcv_adapter;
    private String str_ip;
    private int port;
    private List<String> img_fns;

    static  final int HANDLER_RPC = 2000;
    static  final int HANDLER_RPC_ERR = 2002;

    private RcvSerAdd rcv_ser_add;
    private Bitmap   bitmap_share;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_image_view);

        DisplayMetrics displaymetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(displaymetrics);

        MyApp myApp  =  (MyApp)getApplication();
        myApp.ht = displaymetrics.heightPixels;
        myApp.wt = displaymetrics.widthPixels;
    }

    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS:
                {
                    ImageView image_view = (ImageView) findViewById(R.id.imageView);

                    MyApp my_app = (MyApp) getApplication();
                    mat_show = my_app.mat_rgb;
                    bitmap_share = null;

                    Intent intent = getIntent();
                    String action = intent.getAction();
                    String type = intent.getType();

                    if (Intent.ACTION_SEND.equals(action) && type != null) {
                        if ("text/plain".equals(type)) {
                            handleSendText(intent);
                        } else if (type.startsWith("image/")) {
                            handleSendImage(intent);
                        }
                    }

                    if (bitmap_share != null) Utils.bitmapToMat(bitmap_share, mat_show);

                    int  w_re = (int)(my_app.wt*0.7 + 0.5);
                    mat_resize = image_resize(mat_show, w_re);
                    mat_rpc = image_resize(mat_show, 400);
                    mat_rpc = mat_show;

                    Bitmap bitmap = Bitmap.createBitmap(mat_resize.cols(), mat_resize.rows(), Bitmap.Config.RGB_565);
                    Utils.matToBitmap(mat_resize, bitmap);
                    image_view.setImageBitmap(bitmap);

                    // Get GridView from xml
                    GridView gridView = (GridView) findViewById(R.id.gridView);
                    rcv_adapter = new RcvGridAdapter(RcvGridActivity.this, my_app.wt, my_app.ht, 9);
                    // Set Adapter for GridView
                    gridView.setAdapter(rcv_adapter);
                    img_match();

                } break;
                default:
                {
                    super.onManagerConnected(status);
                } break;
            }
        }
    };

    @Override
    public void onResume()
    {
        super.onResume();
        OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_2_4_8, this, mLoaderCallback);
    }

    Handler handler_rpc = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case HANDLER_RPC:
                    rcv_adapter.notifyDataSetChanged();
                    break;
                case HANDLER_RPC_ERR:
                    String str_txt = msg.getData().getString("debug");
                    Toast.makeText(RcvGridActivity.this, str_txt, Toast.LENGTH_LONG).show();
                    break;
            }
        }
    };

    class  MyException extends Exception
    {
        public MyException(String msg)
        {
            super(msg) ;
        }
    }

    public void img_match() {
        new Thread(new Runnable() {
            public void run() {
                String  str_debug = null;
                try {
                    //MyApp my_app = (MyApp) getApplication();
                    rcv_ser_add = RcvGridAdapter.get_ser_ipv4();

                    if (rcv_ser_add == null) {
                        throw new MyException("get_ser_ipv4 error !!");
                    }

                    str_ip = rcv_ser_add.ip_v4;
                    port = Integer.parseInt(rcv_ser_add.port);

                    TTransport transport;
                    transport = new TSocket(str_ip, port);
                    transport.open();

                    TProtocol protocol = new TBinaryProtocol(transport);
                    TranData.Client client = new TranData.Client(protocol);

                    MatOfInt params90 = new MatOfInt(Highgui.IMWRITE_JPEG_QUALITY, 90);
                    MatOfByte buff90 = new MatOfByte();

                    Highgui.imencode(".jpg", mat_rpc, buff90, params90);

                    byte[] img_bytesa = buff90.toArray();
                    ByteBuffer img_buf01 = ByteBuffer.wrap(img_bytesa);

                    String fun_name = "match";
                    Map<String, String> pa = new HashMap<String, String>();
                    List<String> pa_match = new ArrayList<String>();
                    img_fns = client.image_match(fun_name, img_buf01, pa_match);

                    Bitmap bmp = BitmapFactory.decodeResource(getResources(),R.drawable.test01);
                    Mat  mat_null_img = new Mat();
                    Utils.bitmapToMat(bmp, mat_null_img);

                    for (int i=0; i<img_fns.size(); i++){
                        String img_fn = img_fns.get(i);
                        ByteBuffer img_buf = client.read_image(img_fn, pa);

                        byte[] img_bytes = new byte[img_buf.limit()];
                        img_buf.get(img_bytes);
                        int size_buf = img_buf.limit();
                        MatOfByte mat_bytes = new MatOfByte();
                        mat_bytes.fromArray(img_bytes);

                        //Size msize =  rcv_adapter.mat_show_rpcs.get(i).size();
                        //Mat  mat_null_img = new Mat(msize, CvType.CV_8UC3, new Scalar(250.0, 0.0, 0.0));
                        //Mat  mat_null_img = new Mat();

                        if (size_buf != 0) {
                            Mat mat_show = Highgui.imdecode(mat_bytes, Highgui.CV_LOAD_IMAGE_COLOR);
                            Imgproc.cvtColor(mat_show, mat_show, Imgproc.COLOR_BGR2RGB);
                            rcv_adapter.mat_show_rpcs.set(i, mat_show);
                        } else {
                            str_debug = "rcv exception !";
                            //Imgproc.cvtColor(mat_null_img, rcv_adapter.mat_show_rpcs.get(i), Imgproc.COLOR_BGR2RGB);
                            rcv_adapter.mat_show_rpcs.set(i, mat_null_img);
                            //break;
                        }
                    }
                    transport.close();
                } catch (MyException e) {
                    e.printStackTrace();
                    str_debug = "udp find rcv error !";
                } catch (TException e) {
                    if (e instanceof TApplicationException  && ((TApplicationException) e).getType() == TApplicationException.MISSING_RESULT) {
                        Log.d("rcv debug","The result of function is NULL");
                        str_debug = "rcv result NULL !";
                    } else {
                        e.printStackTrace();
                        str_debug = "rcv exception !";
                    }
                }

                Message msg = new Message();
                msg.what = HANDLER_RPC;
                handler_rpc.sendMessage(msg);

                if (str_debug != null)  {
                    Message msg_err = new Message();
                    msg_err.what = HANDLER_RPC_ERR;
                    Bundle bundle = new Bundle();
                    bundle.putString("debug", str_debug);
                    msg_err.setData(bundle);
                    handler_rpc.sendMessage(msg_err);
                }
            }
        }).start();
    }

    void handleSendText(Intent intent) {
        String sharedText = intent.getStringExtra(Intent.EXTRA_TEXT);
        if (sharedText != null) {
            Toast.makeText(RcvGridActivity.this, sharedText, Toast.LENGTH_LONG).show();
        }
    }

    void handleSendImage(Intent intent) {

        Uri imageUri = (Uri) intent.getParcelableExtra(Intent.EXTRA_STREAM);
        Toast.makeText(RcvGridActivity.this, imageUri.toString(), Toast.LENGTH_LONG).show();
        ContentResolver cr = this.getContentResolver();
        try {
            bitmap_share = BitmapFactory.decodeStream(cr.openInputStream(imageUri));
        } catch (FileNotFoundException e) {
            Log.e("Exception", e.getMessage(), e);
        }
    }

}

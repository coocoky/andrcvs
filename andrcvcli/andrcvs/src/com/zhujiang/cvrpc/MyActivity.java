package com.zhujiang.cvrpc;

import android.app.Activity;
import android.content.ContentResolver;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.GridView;
import android.app.Activity;
import android.content.Intent;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.LinearLayout;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Date;
import java.util.Enumeration;
import android.net.DhcpInfo;
import android.net.wifi.WifiManager;
import android.widget.Toast;

import org.json.JSONException;
import org.json.JSONObject;

import  org.opencv.android.BaseLoaderCallback;
import  org.opencv.android.LoaderCallbackInterface;
import  org.opencv.android.OpenCVLoader;
import org.opencv.android.Utils;
import org.opencv.core.Mat;
import org.opencv.core.Size;
import org.opencv.highgui.Highgui;
import org.opencv.imgproc.Imgproc;

import static android.os.SystemClock.sleep;

class  RcvSerAdd{
    String  ip_v4;
    String  port;
}

class CompratorByLastModified implements Comparator<File>
{
    public int compare(File f1, File f2) {
        long diff = f1.lastModified()-f2.lastModified();
        if(diff>0)
            return 1;
        else if(diff==0)
            return 0;
        else
            return -1;
    }
    public boolean equals(Object obj){
        return true;
    }
}

public class MyActivity extends Activity {

    private static final String  LOG_TAG = "andrcvs";
    static final int  HANDLER_UDP = 1000;
    static final int  HANDLER_SHARE = 1002;

    Handler h_udp;

    GridView grid_view;
    ImageAdapter  image_adapter;
    int     idx_bmp;
    RcvSerAdd rcv_ser_add;
    String   str_debug;
    Bitmap   bitmap_share;

    public Integer[] res_imgs = { R.drawable.image_0001, R.drawable.image_0002, R.drawable.image_0003,
            R.drawable.image_0004, R.drawable.image_0005, R.drawable.image_0006,
            R.drawable.image_0007, R.drawable.image_0008, R.drawable.image_0009,
            R.drawable.image_0010, R.drawable.image_0011, R.drawable.image_0012
    };


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_my);
    }

    private void init_view() {
        DisplayMetrics displaymetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(displaymetrics);

        MyApp myApp  =  (MyApp)getApplication();

        myApp.ht = displaymetrics.heightPixels;
        myApp.wt = displaymetrics.widthPixels;

        image_adapter = new ImageAdapter(this, myApp.wt, myApp.ht, 12);

        // Get GridView from xml
        //final GridView grid_view = (GridView) findViewById(R.id.gridView);
        grid_view = (GridView) findViewById(R.id.gridView);

        try {
            if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
                File sdCardDir = Environment.getExternalStorageDirectory();
                String str_dir = sdCardDir.getCanonicalPath();

                image_adapter.str_path = str_dir + "/andrcvs/";
                File dir = new File(image_adapter.str_path);
                File [] fs = dir.listFiles();
                Arrays.sort(fs, new CompratorByLastModified());

                image_adapter.str_fns = new ArrayList<String>();

                for (int i =  fs.length - 1; i >= 0; i--) {
                   // System.out.println(new Date(fs[i].lastModified()).toLocaleString());
                    image_adapter.str_fns.add(fs[i].getName());
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        // Set Adapter for GridView
        grid_view.setAdapter(image_adapter);
        /**
         * On Click event for Single GridView Item
         * */
        grid_view.setOnItemClickListener(new OnItemClickListener()
        {
            @Override
            public void onItemClick(AdapterView<?> parent, View v, int position, long id)
            {
                //int post01 = grid_view.getFirstVisiblePosition();
                //ImageView imageView = (ImageView)grid_view.getChildAt(position - post01);
                //Bitmap bitmap = ((BitmapDrawable)imageView.getDrawable()).getBitmap();
                //Utils.bitmapToMat(bitmap, mat_rgb);

                if (position >= image_adapter.str_fns.size()) return;

                String  str_fn = image_adapter.str_path + image_adapter.str_fns.get(position);
                Mat  mat_rgb = Highgui.imread(str_fn);
                if (mat_rgb == null) return;

                Imgproc.cvtColor(mat_rgb, mat_rgb, Imgproc.COLOR_BGR2RGB);
                MyApp myApp  =  (MyApp)getApplication();
                myApp.mat_rgb = mat_rgb;

                // Create new intent
                Intent i = new Intent(MyActivity.this, RcvGridActivity.class);
                // Send Image ID to ImageActivity
                //i.putExtra("id", position);
                startActivity(i);
            }
        });
    }

    public void  init_sd_imgs() {
        try
        {
            if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED))
            {
                File sdCardDir = Environment.getExternalStorageDirectory();
                String  str_sd_dir = sdCardDir.getCanonicalPath();
                String  str_path = str_sd_dir + "/andrcvs/";
                File dir = new File(str_path);
                Toast.makeText(MyActivity.this, str_path, Toast.LENGTH_LONG).show();

                if (!dir.exists()) {
                    dir.mkdirs();
                    for (int idx = 0; idx < res_imgs.length; idx++) {
                        Bitmap bmp = BitmapFactory.decodeResource(getResources(), res_imgs[idx]);
                        Mat mat_rgb = new Mat();
                        Utils.bitmapToMat(bmp, mat_rgb);
                        String str_idx = String.format("%04d", idx + 1);
                        String str_fn = str_path + "image_" + str_idx + ".jpg";
                        Imgproc.cvtColor(mat_rgb, mat_rgb, Imgproc.COLOR_BGR2RGB);
                        Highgui.imwrite(str_fn, mat_rgb);
                    }
                }
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {

        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS:
                {
                    init_sd_imgs();
                    init_view();
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

}

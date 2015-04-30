package com.example.andrcvs;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.net.DhcpInfo;
import android.net.wifi.WifiManager;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.ImageView;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;

import org.json.JSONException;
import org.json.JSONObject;
import org.opencv.android.Utils;
import org.opencv.core.Mat;
import org.opencv.core.Size;
import org.opencv.imgproc.Imgproc;

import java.util.ArrayList;
import java.util.List;
import java.util.Vector;

public class RcvGridAdapter extends BaseAdapter {
    private   static  final String  TAG = "andrcvs";
    //private  View  m_view;
    private   Context mContext;
    //private  int ht;
    private   int wt;
    private   int n_mats;
    public    Vector<Mat>  mat_show_rpcs;

    public RcvGridAdapter(Context c, int w, int h) {
        mContext = c;
        wt = w;
        //ht = h;
        //n_mats = n;
        mat_show_rpcs = new Vector<Mat>();
    }

    public  void  set_n_mats(int n) {
        n_mats = n;
    }

    public static Mat image_resize(Mat mat_src, int wh) {
        double dw = mat_src.cols();
        double dh = mat_src.rows();
        double dw01, dh01;
        double scale;

        Mat mat_resize = new Mat();

        if (dw > dh) {
            scale = wh / dw;
            dw01 = wh;
            dh01 = dh * scale;
        } else {
            scale = wh / dh;
            dh01 = wh;
            dw01 = dw * scale;
        }

        Imgproc.resize(mat_src, mat_resize, new Size(dw01+0.5, dh01+0.5), 0.0, 0.0, Imgproc.INTER_CUBIC );
        return mat_resize;
    }

    @Override
    public int getCount()
    {
        return n_mats;
    }
    @Override
    public Object getItem(int position)
    {
        return mat_show_rpcs.get(position);
    }

    @Override
    public long getItemId(int position) {
        // TODO Auto-generated method stub
        return 0;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent)
    {
        ImageView imageView;
        if(convertView == null)
        // Recycled View
        {
            imageView = new ImageView(mContext);
            imageView.setScaleType(ImageView.ScaleType.FIT_CENTER);
            int  wh = (int)((wt*0.7)/3.0 + 0.5);
            imageView.setLayoutParams(new GridView.LayoutParams( wh,  wh));
        }
        else // Re-use the view
        {
            imageView = (ImageView) convertView;
        }

        Mat   mat_show = null;
        synchronized(mat_show_rpcs) {
            if (mat_show_rpcs.size() > position){
                mat_show = mat_show_rpcs.get(position);
            }
        }

        if (mat_show == null) {
            imageView.setImageResource(R.drawable.opencv_01);
        }
        else {
            int  wh_re = (int)((wt*0.7)/3.0 + 0.5);
            Mat   mat_resize = RcvGridAdapter.image_resize(mat_show, wh_re);
            Bitmap bmp_show = Bitmap.createBitmap(mat_resize.cols(), mat_resize.rows(), Bitmap.Config.ARGB_8888);
            Utils.matToBitmap(mat_resize, bmp_show);
            imageView.setImageBitmap(bmp_show);
        }
        return imageView;
    }
}

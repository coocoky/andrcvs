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

public class RcvGridAdapter extends BaseAdapter {
    private  static  final String  TAG = "andrcvs";
    //private  View  m_view;
    private  Context mContext;
    //private  int ht;
    private  int wt;
    private  int n_mats;
    public    List<Mat> mat_show_rpcs;

    public RcvGridAdapter(Context c, int w, int h, int n) {
        mContext = c;
        wt = w;
        //ht = h;
        n_mats = n;
        mat_show_rpcs = new ArrayList<Mat>();
        for (int i=0; i<n_mats; i++){
            mat_show_rpcs.add(new Mat());
        }
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

        Imgproc.resize(mat_src, mat_resize, new Size(dw01, dh01));
        return mat_resize;
    }

    private static String get_local_ip(){
        try {
            for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces();
                 en.hasMoreElements();) {
                NetworkInterface intf = en.nextElement();
                for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements();) {
                    InetAddress inetAddress = enumIpAddr.nextElement();
                    if (!inetAddress.isLoopbackAddress()) {
                        return inetAddress.getHostAddress().toString();
                    }
                }
            }
        } catch (SocketException ex){
            Log.e(TAG, ex.toString());
        }
        return null;
    }

    public static RcvSerAdd get_ser_ipv4() {
        try {
            int server_port = 9092;
            DatagramSocket s = new DatagramSocket();
            s.setBroadcast(true);
            s.setSoTimeout(800);
            int android_port = s.getLocalPort();

            RcvSerAdd  rcv_ser_add = new RcvSerAdd();
            JSONObject object = new JSONObject();
            //String  str_ip = RcvGridAdapter.get_local_ip();

            try {
                object.put("name", "cvrpc");
                //object.put("ipaddress", RcvGridAdapter.get_local_ip());
                object.put("ipaddress", "0.0.0.0");
                object.put("port", android_port);
            } catch (JSONException e) {
                Log.d("debug", e.toString());
            }

            // broadcast discovery packet using current network details
            //InetAddress local = InetAddress.getByName("255.255.255.255");
            InetAddress local = InetAddress.getByName("192.168.0.255");
            Log.d("debug", object.toString());
            //Log.d("debug", local.getHostAddress().toString());

            int msg_length = object.toString().length();
            byte[] message = object.toString().getBytes();
            DatagramPacket p = new DatagramPacket(message, msg_length, local, server_port);
            s.send(p);

            byte[] buffer = new byte[256];

            DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
            s.receive(packet);
            s.close();

            String str_temp = new String(packet.getData() , packet.getOffset() , packet.getLength());
            String  str_ser_name = new String();

            try {
                JSONObject json_serip = new JSONObject(str_temp);
                rcv_ser_add.ip_v4 = json_serip.getString("ipv4");
                rcv_ser_add.port = json_serip.getString("port");
                str_ser_name =  json_serip.getString("name");
            } catch (JSONException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }

            if (!str_ser_name.equals("cvrpc")) rcv_ser_add = null;
            return  rcv_ser_add;

        } catch(SocketException e) {
            Log.d("debug", e.toString());
        } catch(IOException e) {
            Log.d("debug", e.toString());
        }
        return  null;
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
            int  wh = (int)((wt*0.8)/3.0 + 0.5);
            imageView.setLayoutParams(new GridView.LayoutParams( wh,  wh));
        }
        else // Re-use the view
        {
            imageView = (ImageView) convertView;
        }

        Mat   mat_show = mat_show_rpcs.get(position);
        if (mat_show.cols()==0 || mat_show.rows()==0) {
            imageView.setImageResource(R.drawable.test01);
        }
        else {
            int  wh_re = (int)((wt*0.8)/3.0 + 0.5);
            Mat   mat_resize = RcvGridAdapter.image_resize(mat_show, wh_re);
            Bitmap bmp_show = Bitmap.createBitmap(mat_resize.cols(), mat_resize.rows(), Bitmap.Config.RGB_565);
            Utils.matToBitmap(mat_resize, bmp_show);
            imageView.setImageBitmap(bmp_show);
        }
        return imageView;
    }
}

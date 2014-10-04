package com.zhujiang.cvrpc;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Environment;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.ImageView;

import org.opencv.android.Utils;
import org.opencv.core.Mat;
import org.opencv.core.Size;
import org.opencv.highgui.Highgui;
import org.opencv.imgproc.Imgproc;

import java.io.File;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

public class ImageAdapter extends BaseAdapter {

    private Context mContext;
    private int ht;
    private int wt;
    private  int n_mats;

    public  String  str_path;
    public  List<String> str_fns;

    public ImageAdapter(Context c, int w, int h, int n){
        mContext = c;
        wt = w;
        ht = h;
        n_mats = n;
    }

    @Override
    public int getCount()
    {
        return n_mats;
    }
    @Override
    public Object getItem(int position)
    {
        return str_fns.get(position);
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
            int  wh = (int)((wt*0.9)/3.0 + 0.5);
            imageView.setLayoutParams(new GridView.LayoutParams( wh,  wh));
        }
        else // Re-use the view
        {
            imageView = (ImageView) convertView;
        }

        imageView.setImageResource(R.drawable.test01);
        if (position >= str_fns.size() )  return imageView;

        String  str_fn = str_path + str_fns.get(position);
        Mat mat_show = Highgui.imread(str_fn);

        if (mat_show != null) {
            Imgproc.cvtColor(mat_show, mat_show, Imgproc.COLOR_BGR2RGB);
            int  wh_re = (int)((wt*0.9)/3.0 + 0.5);
            Mat   mat_resize = RcvGridAdapter.image_resize(mat_show, wh_re);
            Bitmap bmp = Bitmap.createBitmap(mat_resize.cols(), mat_resize.rows(), Bitmap.Config.RGB_565);
            Utils.matToBitmap(mat_resize, bmp);
            imageView.setImageBitmap(bmp);
        }

        return imageView;
    }
}

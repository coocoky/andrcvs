package com.example.andrcvs;

import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentTransaction;
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
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
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

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
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



public class MainActivity extends Activity {

    Bundle   mState;
    Bitmap   bitmap_share;
    Mat      mat_share;
    int      wt,ht;

    public Integer[] res_imgs = { R.drawable.image_0001, R.drawable.image_0002, R.drawable.image_0003,
            R.drawable.image_0004, R.drawable.image_0005, R.drawable.image_0006,
            R.drawable.image_0007, R.drawable.image_0008, R.drawable.image_0009
    };

	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        DisplayMetrics displaymetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(displaymetrics);

        //MyApp myApp  =  (MyApp)getApplication();

        ht = displaymetrics.heightPixels;
        wt = displaymetrics.widthPixels;

        mState = savedInstanceState;
    }


    private  void handleSendText(Intent intent) {
        String sharedText = intent.getStringExtra(Intent.EXTRA_TEXT);
        if (sharedText != null) {
            Toast.makeText(this, sharedText, Toast.LENGTH_LONG).show();
        }
    }

    private  void handleSendImage(Intent intent) {

        Uri imageUri = (Uri) intent.getParcelableExtra(Intent.EXTRA_STREAM);
        Toast.makeText(this, imageUri.toString(), Toast.LENGTH_LONG).show();
        ContentResolver cr = this.getContentResolver();
        try {
            bitmap_share = BitmapFactory.decodeStream(cr.openInputStream(imageUri));
        } catch (FileNotFoundException e) {
            Log.e("Exception", e.getMessage(), e);
        }
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
                Toast.makeText(MainActivity.this, str_path, Toast.LENGTH_LONG).show();

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

                    mat_share = new Mat();
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

                    if (bitmap_share != null) {

                        Utils.bitmapToMat(bitmap_share, mat_share);
                        //my_app.mat_rgb = mat_show;
                        Fragment fragment =  new RcvGridFragment();
                        FragmentTransaction transaction = getFragmentManager().beginTransaction();
                        transaction.replace(R.id.container, fragment);
                        //transaction.addToBackStack(null);
                        transaction.commit();
                    } else {
                        //init_view();
                        if (mState == null) {
                            getFragmentManager().beginTransaction()
                                    .add(R.id.container, new GalleryFragment())
                                    .commit();
                        }
                    }

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

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();
        if (id == R.id.action_settings) {
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    /**
     * A placeholder fragment containing a simple view.
     */
    public static class PlaceholderFragment extends Fragment {

        public PlaceholderFragment() {
        }

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container,
                Bundle savedInstanceState) {
            View rootView = inflater.inflate(R.layout.fragment_main, container, false);
            return rootView;
        }
    }
}

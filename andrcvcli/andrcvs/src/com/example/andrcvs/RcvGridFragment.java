package com.example.andrcvs;

import android.app.FragmentTransaction;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.app.Fragment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.Toast;

import org.apache.thrift.TApplicationException;
import org.apache.thrift.TException;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TTransport;
import org.opencv.android.Utils;
import org.opencv.core.Mat;
import org.opencv.core.MatOfByte;
import org.opencv.core.MatOfInt;
import org.opencv.highgui.Highgui;
import org.opencv.imgproc.Imgproc;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import cvrpc.TranData;


/**
 * A simple {@link Fragment} subclass.
 */
public class RcvGridFragment extends Fragment {

    private Mat  mat_show;
    private Mat  mat_resize;
    private Mat  mat_rpc;
    private RcvGridAdapter rcv_adapter;
    private String str_ip;
    private int port;
    private List<String> img_fns;
    private ByteBuffer   img_buf_sort_src;

    static  final int HANDLER_RPC = 2000;
    static  final int HANDLER_RPC_ERR = 2002;

    private RcvSerAdd     rcv_ser_add;
    private MainActivity  m_activity;
    private View          view_frame;
    private GridView      grid_view;

    public RcvGridFragment() {
        // Required empty public constructor
    }


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        view_frame = inflater.inflate(R.layout.fragment_rcv_grid, container, false);
        m_activity = (MainActivity)getActivity();
        mat_show = m_activity.mat_share;
        int  w_re = (int)(m_activity.wt*0.7 + 0.5);
        //mat_resize = RcvGridAdapter.image_resize(mat_show, w_re);
        mat_rpc =  m_activity.mat_share;
        //if ( mat_rpc.cols() > 480 ||  mat_rpc.rows() > 480 ) mat_rpc = RcvGridAdapter.image_resize( m_activity.mat_share, 480);
        mat_rpc = RcvGridAdapter.image_resize( m_activity.mat_share, 240);

        // Get GridView from xml
        grid_view = (GridView) view_frame.findViewById(R.id.gridView);
        rcv_adapter = new RcvGridAdapter(m_activity, m_activity.wt, m_activity.ht);
        // Set Adapter for GridView
        grid_view.setAdapter(rcv_adapter);
        rcv_adapter.set_n_mats(12);

        Bitmap  bmp = BitmapFactory.decodeResource(getResources(), R.drawable.test01);
        Mat     mat_null_img = new Mat();
        Utils.bitmapToMat(bmp, mat_null_img);

        /*
        rcv_adapter.mat_show_rpcs.set(0, mat_rpc);
        rcv_adapter.mat_show_rpcs.set(1, mat_null_img);
        rcv_adapter.mat_show_rpcs.set(2, mat_null_img);
        */

        rcv_adapter.mat_show_rpcs.clear();
        //rcv_adapter.mat_show_rpcs = new ArrayList<Mat>();
        rcv_adapter.mat_show_rpcs.add(mat_rpc);
        rcv_adapter.mat_show_rpcs.add(mat_null_img);
        rcv_adapter.mat_show_rpcs.add(mat_null_img);

        grid_view.setOnItemClickListener(new AdapterView.OnItemClickListener()
        {
            @Override
            public void onItemClick(AdapterView<?> parent, View v, int position, long id)
            {
                m_activity.mat_share = rcv_adapter.mat_show_rpcs.get(position);
                if ( m_activity.mat_share.cols() == 0 ||  m_activity.mat_share.rows() == 0) return;

                Fragment fragment = new ImageShowFragment();
                FragmentTransaction transaction = getFragmentManager().beginTransaction();
                transaction.replace(R.id.container, fragment);
                transaction.addToBackStack(null);
                transaction.commit();
            }
        });

        setHasOptionsMenu(true);
        img_match();

        // Inflate the layout for this fragment

        return  view_frame;
    }

    Handler handler_rpc = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case HANDLER_RPC:
                    rcv_adapter.notifyDataSetChanged();
                    break;
                case HANDLER_RPC_ERR:
                    String str_txt = msg.getData().getString("debug");
                    Toast.makeText(m_activity, str_txt, Toast.LENGTH_LONG).show();
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

        //grid_view.setSelection(0);
        grid_view.smoothScrollToPosition(0);

        new Thread(new Runnable() {
            public void run() {
                String  str_debug = null;
                //int clientTimeout = 30*1000;
                TTransport transport;
                try {
                    //MyApp my_app = (MyApp) getApplication();
                    rcv_ser_add = RcvGridAdapter.get_ser_ipv4();

                    if (rcv_ser_add == null) {
                        throw new MyException("get_ser_ipv4 error !!");
                    }

                    str_ip = rcv_ser_add.ip_v4;
                    port = Integer.parseInt(rcv_ser_add.port);

                    transport = new TFramedTransport(new TSocket(str_ip, port));

                    TProtocol protocol = new TBinaryProtocol(transport);
                    TranData.Client client = new TranData.Client(protocol);
                    transport.open();

                    MatOfInt params90 = new MatOfInt(Highgui.IMWRITE_JPEG_QUALITY, 97);
                    MatOfByte buff90 = new MatOfByte();

                    Mat    mat_rgb = new Mat();
                    Imgproc.cvtColor(mat_rpc, mat_rgb, Imgproc.COLOR_BGR2RGB);
                    Highgui.imencode(".jpg", mat_rgb, buff90, params90);

                    byte[] img_bytes = buff90.toArray();
                    ByteBuffer img_buf = ByteBuffer.wrap(img_bytes);

                    //String fun_name = "match";
                    Map<String, String> pa = new HashMap<String, String>();
                    List<String> pa_match = new ArrayList<String>();
                    img_fns = client.image_match(img_buf, pa_match);

                    Bitmap bmp = BitmapFactory.decodeResource(getResources(), R.drawable.test01);
                    Mat  mat_null_img = new Mat();
                    Utils.bitmapToMat(bmp, mat_null_img);

                    rcv_adapter.set_n_mats(img_fns.size()+3);

                    for (int i=0; i<img_fns.size(); i++){
                        String img_fn = img_fns.get(i);
                        img_buf = client.read_image(img_fn, pa);

                        byte[] img_bytes_re = new byte[img_buf.remaining()];
                        img_buf.get(img_bytes_re, 0, img_bytes_re.length);

                        MatOfByte mat_bytes = new MatOfByte();
                        mat_bytes.fromArray(img_bytes_re);

                        Mat   mat_rcv = new Mat();
                        if (img_bytes.length != 0) {
                            mat_rcv = Highgui.imdecode(mat_bytes, Highgui.CV_LOAD_IMAGE_COLOR);
                            Imgproc.cvtColor(mat_rcv, mat_rcv, Imgproc.COLOR_BGR2RGB);
                        } else {
                            str_debug = "rcv exception !";
                            mat_rcv = mat_null_img;
                        }

                        synchronized(rcv_adapter.mat_show_rpcs) {
                            //rcv_adapter.mat_show_rpcs.set(i+3, mat_rcv);
                            rcv_adapter.mat_show_rpcs.add(mat_rcv);
                        }

                        Message msg = new Message();
                        msg.what = HANDLER_RPC;
                        handler_rpc.sendMessage(msg);

                        try {
                            Thread.sleep(20);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }

                    transport.close();
                } catch (MyException e) {
                    e.printStackTrace();
                    str_debug = "udp find rcv error !";
                } catch (TException e) {
                    if (e instanceof TApplicationException && ((TApplicationException) e).getType() == TApplicationException.MISSING_RESULT) {
                        Log.d("rcv debug", "The result of function is NULL");
                        str_debug = "rcv result NULL !";
                    } else {
                        e.printStackTrace();
                        str_debug = "rcv exception !";
                    }
                }

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

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        return super.onOptionsItemSelected(item);
    }

}

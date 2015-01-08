package com.example.andrcvs;

import android.app.Activity;
import android.app.FragmentTransaction;
import android.content.ContentResolver;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.app.Fragment;
import android.os.Handler;
import android.os.Message;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.LayoutInflater;
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

import java.io.FileNotFoundException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import cvrpc.TranData;

import static com.example.andrcvs.RcvGridAdapter.image_resize;


/**
 * A simple {@link Fragment} subclass.
 * Activities that contain this fragment must implement the
 * {@link RcvGridFragment.OnFragmentInteractionListener} interface
 * to handle interaction events.
 * Use the {@link RcvGridFragment#newInstance} factory method to
 * create an instance of this fragment.
 */
public class RcvGridFragment extends Fragment {
    // TODO: Rename parameter arguments, choose names that match
    // the fragment initialization parameters, e.g. ARG_ITEM_NUMBER

    private Mat  mat_show;
    private Mat  mat_resize;
    private Mat  mat_rpc;
    private RcvGridAdapter rcv_adapter;
    private String str_ip;
    private int port;
    private List<String> img_fns;

    static  final int HANDLER_RPC = 2000;
    static  final int HANDLER_RPC_ERR = 2002;

    private RcvSerAdd     rcv_ser_add;
    private MainActivity  m_activity;
    private View          view_frame;

    public RcvGridFragment() {
        // Required empty public constructor
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        view_frame = inflater.inflate(R.layout.fragment_rcv_grid, container, false);

        m_activity = (MainActivity)getActivity();

        ImageView image_view = (ImageView) view_frame.findViewById(R.id.imageView);

        mat_show = m_activity.mat_share;

        int  w_re = (int)(m_activity.wt*0.7 + 0.5);
        mat_resize = RcvGridAdapter.image_resize(mat_show, w_re);
        //mat_rpc = RcvGridAdapter.image_resize(mat_show, 400);
        mat_rpc = mat_show;

        Bitmap bitmap = Bitmap.createBitmap(mat_resize.cols(), mat_resize.rows(), Bitmap.Config.RGB_565);
        Utils.matToBitmap(mat_resize, bitmap);
        image_view.setImageBitmap(bitmap);

        // Get GridView from xml
        GridView grid_view = (GridView) view_frame.findViewById(R.id.gridView);
        rcv_adapter = new RcvGridAdapter(m_activity, m_activity.wt, m_activity.ht, 9);
        // Set Adapter for GridView
        grid_view.setAdapter(rcv_adapter);

        grid_view.setOnItemClickListener(new AdapterView.OnItemClickListener()
        {
            @Override
            public void onItemClick(AdapterView<?> parent, View v, int position, long id)
            {
                //int post01 = grid_view.getFirstVisiblePosition();
                //ImageView imageView = (ImageView)grid_view.getChildAt(position - post01);
                //Bitmap bitmap = ((BitmapDrawable)imageView.getDrawable()).getBitmap();
                //Utils.bitmapToMat(bitmap, mat_rgb);

                //String    str_txt = "ItemClick";
                //Toast.makeText(m_activity, str_txt, Toast.LENGTH_LONG).show();

                m_activity.mat_share = rcv_adapter.mat_show_rpcs.get(position);

                Fragment fragment = new ImageShowFragment();
                FragmentTransaction transaction = getFragmentManager().beginTransaction();
                transaction.replace(R.id.container, fragment);
                transaction.addToBackStack(null);
                transaction.commit();

            }
        });

        img_match();

        // Inflate the layout for this fragment

        return  view_frame;
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
    }

    @Override
    public void onDetach() {
        super.onDetach();
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
        new Thread(new Runnable() {
            public void run() {
                String  str_debug = null;
                int clientTimeout = 30000;
                TTransport transport;
                try {
                    //MyApp my_app = (MyApp) getApplication();
                    rcv_ser_add = RcvGridAdapter.get_ser_ipv4();

                    if (rcv_ser_add == null) {
                        throw new MyException("get_ser_ipv4 error !!");
                    }

                    str_ip = rcv_ser_add.ip_v4;
                    port = Integer.parseInt(rcv_ser_add.port);

                    //TTransport transport;
                    //transport = new TSocket(str_ip, port);

                    transport = new TFramedTransport(new TSocket(str_ip, port));

                    TProtocol protocol = new TBinaryProtocol(transport);
                    TranData.Client client = new TranData.Client(protocol);
                    transport.open();

                    MatOfInt params90 = new MatOfInt(Highgui.IMWRITE_JPEG_QUALITY, 90);
                    MatOfByte buff90 = new MatOfByte();

                    Highgui.imencode(".jpg", mat_rpc, buff90, params90);

                    byte[] img_bytesa = buff90.toArray();
                    ByteBuffer img_buf01 = ByteBuffer.wrap(img_bytesa);

                    String fun_name = "match";
                    Map<String, String> pa = new HashMap<String, String>();
                    List<String> pa_match = new ArrayList<String>();
                    img_fns = client.image_match(fun_name, img_buf01, pa_match);

                    Bitmap bmp = BitmapFactory.decodeResource(getResources(), R.drawable.test01);
                    Mat  mat_null_img = new Mat();
                    Utils.bitmapToMat(bmp, mat_null_img);

                    for (int i=0; i<img_fns.size(); i++){
                        String img_fn = img_fns.get(i);
                        ByteBuffer img_buf = client.read_image(img_fn, pa);

                        byte[] img_bytes = new byte[img_buf.remaining()];
                        img_buf.get(img_bytes, 0, img_bytes.length);
                        //int size_buf = img_buf.limit();
                        //byte[] img_bytes = img_buf.array();
                        MatOfByte mat_bytes = new MatOfByte();
                        mat_bytes.fromArray(img_bytes);

                        if (img_bytes.length != 0) {
                            Mat mat_show = Highgui.imdecode(mat_bytes, Highgui.CV_LOAD_IMAGE_COLOR);
                            Imgproc.cvtColor(mat_show, mat_show, Imgproc.COLOR_BGR2RGB);
                            rcv_adapter.mat_show_rpcs.set(i, mat_show);
                        } else {
                            str_debug = "rcv exception !";

                            rcv_adapter.mat_show_rpcs.set(i, mat_null_img);
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
}

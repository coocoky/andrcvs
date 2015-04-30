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
import org.json.JSONException;
import org.json.JSONObject;
import org.opencv.android.Utils;
import org.opencv.core.Mat;
import org.opencv.core.MatOfByte;
import org.opencv.core.MatOfInt;
import org.opencv.highgui.Highgui;
import org.opencv.imgproc.Imgproc;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Vector;

import cvrpc.TranData;


/**
 * A simple {@link Fragment} subclass.
 */
public class RcvGridFragment extends Fragment {

    private Mat  mat_show;
    //private Mat  mat_resize;
    private Mat  mat_rpc;
    private RcvGridAdapter rcv_adapter;
    private String str_ip;
    private int port;
    private List<String> img_hashs;

    static  final int HANDLER_RPC = 2000;
    static  final int HANDLER_RPC_ERR = 2002;

    private RcvSerAdd     rcv_ser_add;
    private MainActivity  m_activity;
    private View          view_frame;
    private GridView      grid_view;

    public RcvGridFragment(MainActivity  activity) {
        // Required empty public constructor
        m_activity = activity;
        rcv_adapter = new RcvGridAdapter(m_activity, m_activity.wt, m_activity.ht);
    }


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        view_frame = inflater.inflate(R.layout.fragment_rcv_grid, container, false);
        //m_activity = (MainActivity)getActivity();
        mat_show = m_activity.mat_share;
        int  w_re = (int)(m_activity.wt*0.7 + 0.5);
        mat_rpc =  m_activity.mat_share;
        mat_rpc = RcvGridAdapter.image_resize( m_activity.mat_share, 240);

        // Get GridView from xml
        grid_view = (GridView) view_frame.findViewById(R.id.gridView);
        // Set Adapter for GridView
        grid_view.setAdapter(rcv_adapter);
        rcv_adapter.set_n_mats(12);

        Bitmap  bmp = BitmapFactory.decodeResource(getResources(), R.drawable.opencv_01);
        Mat     mat_null_img = new Mat();
        Utils.bitmapToMat(bmp, mat_null_img);

        rcv_adapter.mat_show_rpcs.clear();
        rcv_adapter.mat_show_rpcs = new Vector<Mat>();
        rcv_adapter.mat_show_rpcs.add(mat_rpc);
        rcv_adapter.mat_show_rpcs.add(mat_null_img);
        rcv_adapter.mat_show_rpcs.add(mat_null_img);

        grid_view.setOnItemClickListener(new AdapterView.OnItemClickListener()
        {
            @Override
            public void onItemClick(AdapterView<?> parent, View v, int position, long id)
            {

                if (position >= rcv_adapter.mat_show_rpcs.size() ) {
                    return;
                }

                m_activity.mat_share = rcv_adapter.mat_show_rpcs.get(position);

                if ( m_activity.mat_share.cols() == 0 ||  m_activity.mat_share.rows() == 0) {
                    return;
                }

                rcv_adapter.mat_show_rpcs.clear();
                rcv_adapter.mat_show_rpcs = new Vector<Mat>();

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

    RcvSerAdd  get_ser_ipv4() {
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

    static class MrcvHandler extends Handler {
        WeakReference<RcvGridFragment>  m_fragment;

        MrcvHandler(RcvGridFragment fragment) {
            m_fragment = new WeakReference<RcvGridFragment>(fragment);
        }

        @Override
        public void handleMessage(Message msg) {
            RcvGridFragment fragment = m_fragment.get();
            switch (msg.what) {
                case HANDLER_RPC:
                    fragment.rcv_adapter.notifyDataSetChanged();
                    break;
                case HANDLER_RPC_ERR:
                    String str_txt = msg.getData().getString("debug");
                    Toast.makeText(fragment.m_activity, str_txt, Toast.LENGTH_LONG).show();
                    break;
            }
        }
    };

    MrcvHandler handler_rpc = new MrcvHandler(this);

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
                Bitmap bmp = BitmapFactory.decodeResource(getResources(), R.drawable.opencv_01);
                //int clientTimeout = 30*1000;
                TTransport transport;
                try {
                    //MyApp my_app = (MyApp) getApplication();
                    rcv_ser_add = get_ser_ipv4();

                    if (rcv_ser_add == null) {
                        throw new MyException("get_ser_ipv4 error !!");
                    }

                    str_ip = rcv_ser_add.ip_v4;
                    port = Integer.parseInt(rcv_ser_add.port);

                    transport = new TFramedTransport(new TSocket(str_ip, port));

                    TProtocol protocol = new TBinaryProtocol(transport);
                    TranData.Client client = new TranData.Client(protocol);
                    transport.open();

                    MatOfInt params90 = new MatOfInt(Highgui.IMWRITE_JPEG_QUALITY, 92);
                    MatOfByte buff90 = new MatOfByte();

                    Mat    mat_rgb = new Mat();
                    Imgproc.cvtColor(mat_rpc, mat_rgb, Imgproc.COLOR_BGR2RGB);
                    Highgui.imencode(".jpg", mat_rgb, buff90, params90);

                    byte[] img_bytes = buff90.toArray();
                    ByteBuffer img_buf = ByteBuffer.wrap(img_bytes);

                    //String fun_name = "match";
                    Map<String, String> pa = new HashMap<String, String>();
                    List<String> pa_match = new ArrayList<String>();
                    img_hashs = client.image_match(img_buf, pa_match);

                    Mat  mat_null_img = new Mat();
                    Utils.bitmapToMat(bmp, mat_null_img);
                    rcv_adapter.set_n_mats(img_hashs.size()+3);

                    for (int i=0; i<img_hashs.size(); i++){
                        String img_fn = img_hashs.get(i);
                        pa.put("wh","800");
                        pa.put("jpg","90");
                        img_buf = client.read_image(img_fn, pa);

                        byte[] img_bytes_re = new byte[img_buf.remaining()];
                        img_buf.get(img_bytes_re, 0, img_bytes_re.length);

                        MatOfByte mat_bytes = new MatOfByte();
                        mat_bytes.fromArray(img_bytes_re);

                        Mat   mat_rcv = new Mat();
                        if (img_bytes_re.length != 0) {
                            mat_rcv = Highgui.imdecode(mat_bytes, Highgui.CV_LOAD_IMAGE_COLOR);
                            Imgproc.cvtColor(mat_rcv, mat_rcv, Imgproc.COLOR_BGR2RGB);
                        } else {
                            str_debug = "rcv exception !";
                            mat_rcv = mat_null_img;
                        }

                        synchronized(rcv_adapter.mat_show_rpcs) {
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

package com.example.andrcvs;


import android.app.FragmentTransaction;
import android.os.Bundle;
import android.app.Fragment;
import android.os.Environment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.GridView;

import org.opencv.core.Mat;
import org.opencv.highgui.Highgui;
import org.opencv.imgproc.Imgproc;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;


/**
 * A simple {@link Fragment} subclass.
 */
public class GalleryFragment extends Fragment {
    private static final String  LOG_TAG = "andrcvs";
    static final int  HANDLER_UDP = 1000;
    static final int  HANDLER_SHARE = 1002;

    MainActivity    m_activity;
    GridView        grid_view;
    ImageAdapter    image_adapter;
    View            root_view;

    public GalleryFragment() {
        // Required empty public constructor
    }


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        root_view = inflater.inflate(R.layout.fragment_gallery, container, false);
        m_activity = (MainActivity)getActivity();

        image_adapter = new ImageAdapter(m_activity, m_activity.wt, m_activity.ht, 12);

        // Get GridView from xml
        //final GridView grid_view = (GridView) findViewById(R.id.gridView);
        grid_view = (GridView) root_view.findViewById(R.id.gridView);

        image_adapter.str_fns = new ArrayList<String>();
        m_activity.mat_share = null;

        init_view();
        return  root_view;
    }


    private void init_view() {

        try {
            if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
                File sdCardDir = Environment.getExternalStorageDirectory();
                String str_dir = sdCardDir.getCanonicalPath();

                image_adapter.str_path = str_dir + "/andrcvs/";
                File dir = new File(image_adapter.str_path);
                File [] fs = dir.listFiles();
                Arrays.sort(fs, new CompratorByLastModified());

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
        grid_view.setOnItemClickListener(new AdapterView.OnItemClickListener()
        {
            @Override
            public void onItemClick(AdapterView<?> parent, View v, int position, long id)
            {

                if (position >= image_adapter.str_fns.size()) return;

                String  str_fn = image_adapter.str_path + image_adapter.str_fns.get(position);
                Mat mat_rgb = Highgui.imread(str_fn);
                if (mat_rgb == null) return;

                Imgproc.cvtColor(mat_rgb, mat_rgb, Imgproc.COLOR_BGR2RGB);
                //MyApp myApp  =  (MyApp)getApplication();
                //myApp.mat_rgb = mat_rgb;

                m_activity.mat_share = mat_rgb;

                Fragment fragment =  new RcvGridFragment(m_activity);

                FragmentTransaction transaction = getFragmentManager().beginTransaction();
                transaction.replace(R.id.container, fragment);
                transaction.addToBackStack(null);
                transaction.commit();

                // Create new intent
                //Intent i = new Intent(MyActivity.this, RcvGridActivity.class);
                // Send Image ID to ImageActivity
                //i.putExtra("id", position);
                //startActivity(i);
            }
        });
    }

}

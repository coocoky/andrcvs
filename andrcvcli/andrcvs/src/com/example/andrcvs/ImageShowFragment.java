package com.example.andrcvs;

import android.app.Activity;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import org.opencv.android.Utils;


/**
 * A simple {@link Fragment} subclass.
 * Activities that contain this fragment must implement the
 * {@link ImageShowFragment.OnFragmentInteractionListener} interface
 * to handle interaction events.
 * Use the {@link ImageShowFragment#newInstance} factory method to
 * create an instance of this fragment.
 */
public class ImageShowFragment extends Fragment {
    // TODO: Rename parameter arguments, choose names that match
    // the fragment initialization parameters, e.g. ARG_ITEM_NUMBER

    public ImageShowFragment() {
        // Required empty public constructor
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View  root_view = inflater.inflate(R.layout.fragment_image_show, container, false);
        ImageView image_view = (ImageView) root_view.findViewById(R.id.imageView2);

        MainActivity  m_activity = (MainActivity)getActivity();

        Bitmap bmp_show = Bitmap.createBitmap( m_activity.mat_share.cols(),  m_activity.mat_share.rows(), Bitmap.Config.RGB_565);
        Utils.matToBitmap( m_activity.mat_share, bmp_show);
        image_view.setImageBitmap(bmp_show);

        return root_view;
    }


    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        /*
        try {
            mListener = (OnFragmentInteractionListener) activity;
        } catch (ClassCastException e) {
            throw new ClassCastException(activity.toString()
                    + " must implement OnFragmentInteractionListener");
        }
        */
    }

    @Override
    public void onDetach() {
        super.onDetach();
        
    }

}

package com.zhujiang.cvrpc;

import android.app.Application;
import android.net.DhcpInfo;
import android.net.wifi.WifiManager;
import android.util.Log;
import java.io.IOException;
import java.net.InetAddress;
import org.opencv.core.Mat;

public  class  MyApp  extends  Application
{
    private  static   final String  TAG = "andrcvs";
    public   Mat       mat_rgb;
    public   int       wt,ht;

    public String getIPAddress() throws IOException {
        WifiManager myWifiManager;
        myWifiManager = (WifiManager)getSystemService(WIFI_SERVICE);
        DhcpInfo myDhcpInfo = myWifiManager.getDhcpInfo();
        if (myDhcpInfo == null) {
            Log.e(TAG, "Could not get broadcast address");
            return null;
        }
        int broadcast = myDhcpInfo.ipAddress;
        byte[] quads = new byte[4];
        for (int k = 0; k < 4; k++)
            quads[k] = (byte) ((broadcast >> k * 8) & 0xFF);
        return InetAddress.getByAddress(quads).getHostAddress();
    }
}
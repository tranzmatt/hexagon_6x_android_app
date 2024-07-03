/*==============================================================================
  Copyright (c) 2021,2022 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/

package com.example.android_app;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ToggleButton;

import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;


public class MainActivity extends AppCompatActivity {

    private static final String TAG = "AA.MainActivity";
    private static final List<SessionConfigs> sessionConfigsList= new ArrayList<SessionConfigs>(2);
    private static final List<SessionStats> sessionStatsList= new ArrayList<SessionStats>(2);

    static {
        System.loadLibrary("android_app_apk");
        sessionStatsList.add(new SessionStats(1));
        sessionStatsList.add(new SessionStats(2));
    }

    public native long launch(SessionConfigs app1Settings, SessionStats app1Stats, SessionConfigs app2Settings, SessionStats app2Stats, String libPath, String logPath);
    public native long terminate();

    interface Callback {
        void callback();
    }

    class DoneRunningApplicationCallback implements Callback {
        public void callback() {
            Button beginButton = findViewById(R.id.beginButton);
            beginButton.setText("Launch (new stats available)");
        }
    }

    class MyThread implements Runnable {
        Callback c;
        public MyThread(Callback c) {
            this.c = c;
        }
        public void run() {

            String libPath = getApplicationInfo().nativeLibraryDir;
            Log.i(TAG, "libPath : " + libPath );
            String logPath =getExternalCacheDir().getAbsolutePath() ;
            Log.i(TAG, "logPath : " + logPath );
            launch(sessionConfigsList.get(0), sessionStatsList.get(0), sessionConfigsList.get(1), sessionStatsList.get(1),libPath, logPath);
            this.c.callback(); // callback
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "Begin on Create");
        Intent intent = getIntent();
        Bundle settingsBundle = intent.getBundleExtra(SessionConfigs.KEY_CONFIGS);
        if (settingsBundle==null) {
            /* Default application settings */
            sessionConfigsList.add(new SessionConfigs(1, SessionConfigs.TaskTrigger.TIMED,256,100).setNewNumTasks(1));
            sessionConfigsList.add(new SessionConfigs(2, SessionConfigs.TaskTrigger.QUEUED, 512, 120).setNewMegaCyclesPerTask(50).setNewNumTasks(1));
            Log.i(TAG, "Created default application settings");
        } else {
            SessionConfigs sessionConfigs= new SessionConfigs(settingsBundle);
            // On/off configuration is handled in MainActivity only
            sessionConfigs.setOff(sessionConfigsList.get(sessionConfigs.getSessionId()-1).isOff());
            sessionConfigsList.set(sessionConfigs.getSessionId()-1, sessionConfigs);
        }
        setContentView(R.layout.activity_main);

        ToggleButton app1ToggleButton =  findViewById(R.id.app1OnOff);
        app1ToggleButton.setChecked(!sessionConfigsList.get(0).isOff());
        ToggleButton app2ToggleButton =  findViewById(R.id.app2OnOff);
        app2ToggleButton.setChecked(!sessionConfigsList.get(1).isOff());

        createFarfFile();
    }


    @Override
    protected void onStop() {
        super.onStop();
        updateMainActivityConfigs();
    }


        /** Called when user wants to access Application 1 settings */
    public void manageSettingsApp1(View view) {
        Intent intent = new Intent(this, ConfigsActivity.class);
        SessionConfigs sessionConfigs= (SessionConfigs) sessionConfigsList.get(0);
        intent.putExtra(SessionConfigs.KEY_CONFIGS, sessionConfigs.getBundle());
        startActivity(intent);
    }

    /** Called when user wants to access Application 2 settings */
    public void manageSettingsApp2(View view) {
        Intent intent = new Intent(this, ConfigsActivity.class);
        SessionConfigs sessionConfigs= (SessionConfigs) sessionConfigsList.get(1);
        intent.putExtra(SessionConfigs.KEY_CONFIGS, sessionConfigs.getBundle());
        startActivity(intent);
    }

    /** Called when user wants to begin processing */
    public void beginProcessing(View view) {
        updateMainActivityConfigs();  // Force the change on the main UI to be accounted for
        Button beginButton = findViewById(R.id.beginButton);
        beginButton.setText("Processing...");
        Thread t = new Thread(new MyThread(new DoneRunningApplicationCallback()));
        t.start();
    }

    /** Called when user wants to begin processing */
    public void terminate(View view) {
        terminate();
    }


    /** Called when user wants to access stats from the last run */
    public void getStats(View view) {
        Log.i(TAG, "Begin getStats");
        Intent intent = new Intent(this, StatsActivity.class);
        intent.putExtra(SessionStats.KEY_STATS_APP_1, sessionStatsList.get(0).getBundle());
        intent.putExtra(SessionStats.KEY_STATS_APP_2, sessionStatsList.get(1).getBundle());
        Log.i(TAG, "Start activity stats");
        startActivity(intent);
        Button beginButton = findViewById(R.id.beginButton);
        beginButton.setText("Launch");
    }

    private void updateMainActivityConfigs() {
        ToggleButton app1ToggleButton =  findViewById(R.id.app1OnOff);
        sessionConfigsList.get(0).setOff(!app1ToggleButton.isChecked());
        ToggleButton app2ToggleButton =  findViewById(R.id.app2OnOff);
        sessionConfigsList.get(1).setOff(!app2ToggleButton.isChecked());
    }

    private void createFarfFile() {
        String farfFileName=getExternalCacheDir().getAbsolutePath() + getClass().getPackage().getName() + ".farf";
        try {
            FileWriter myWriter = new FileWriter(farfFileName);
            myWriter.write("0x1f");
            myWriter.close();
        } catch (IOException e) {
            System.out.println("Unable to create farf file " + farfFileName + " to capture dsp messages in logcat");
            e.printStackTrace();
        }
    }


}

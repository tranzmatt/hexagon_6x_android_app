/*==============================================================================
  Copyright (c) 2021,2022 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/

package com.example.android_app;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import java.text.DecimalFormat;
import java.text.NumberFormat;


public class StatsActivity extends AppCompatActivity {

    private static final String TAG = "AA.StatsActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.stats);

        NumberFormat numberFormatter = new DecimalFormat("###,###,###.##");
        NumberFormat percentFormatter = new DecimalFormat("%##.#");

        Intent intent = getIntent();
        Bundle statsApp1Bundle = intent.getBundleExtra(SessionStats.KEY_STATS_APP_1);
        SessionStats statsApp1 = new SessionStats(statsApp1Bundle);

        TextView avgTaskPerSecondView1 = findViewById(R.id.avgTaskPerSecondView1);
        avgTaskPerSecondView1.setText(numberFormatter.format(statsApp1.getAvgNumTasksCompletedPerSecond()));

        TextView percentActiveProcessingView1 = findViewById(R.id.percentActiveProcessingView1);
        percentActiveProcessingView1.setText(percentFormatter.format(statsApp1.getPercentActiveProcessing()));

        TextView percentWaitingOnResourceView1 = findViewById(R.id.percentWaitingOnResourceView1);
        percentWaitingOnResourceView1.setText(percentFormatter.format(statsApp1.getPercentDspTimeWaitingOnResource()));

        TextView numTasksCompletedView1 = findViewById(R.id.numTasksCompletedView1);
        numTasksCompletedView1.setText(Integer.toString(statsApp1.getNumTasksCompleted()));

        TextView numTasksInterruptedView1 = findViewById(R.id.numTasksInterruptedView1);
        numTasksInterruptedView1.setText(Integer.toString(statsApp1.getNumTasksInterrupted()));

        TextView numTasksFailedView1 = findViewById(R.id.numTasksFailedView1);
        numTasksFailedView1.setText(Integer.toString(statsApp1.getNumTasksFailed()));

        TextView numTasksAbortedView1 = findViewById(R.id.numTasksAbortedView1);
        numTasksAbortedView1.setText(Integer.toString(statsApp1.getNumTasksAborted()));

        TextView cpuTimeView1 = findViewById(R.id.cpuTimeView1);
        cpuTimeView1.setText(numberFormatter.format(statsApp1.getCpuTimeUs()));

        TextView dspTimeView1 = findViewById(R.id.dspTimeView1);
        dspTimeView1.setText(numberFormatter.format(statsApp1.getDspTimeUs()));

        TextView fastrpcAvgTimeView1 = findViewById(R.id.fastrpcAvgTimeView1);
        fastrpcAvgTimeView1.setText(numberFormatter.format((int)statsApp1.getFastrpcOverheadPerCall()));


        Bundle statsApp2Bundle = intent.getBundleExtra(SessionStats.KEY_STATS_APP_2);
        SessionStats statsApp2 = new SessionStats(statsApp2Bundle);

        TextView avgTaskPerSecondView2 = findViewById(R.id.avgTaskPerSecondView2);
        avgTaskPerSecondView2.setText(numberFormatter.format(statsApp2.getAvgNumTasksCompletedPerSecond()));

        TextView percentActiveProcessingView2 = findViewById(R.id.percentActiveProcessingView2);
        percentActiveProcessingView2.setText(percentFormatter.format(statsApp2.getPercentActiveProcessing()));

        TextView percentWaitingOnResourceView2 = findViewById(R.id.percentWaitingOnResourceView2);
        percentWaitingOnResourceView2.setText(percentFormatter.format(statsApp2.getPercentDspTimeWaitingOnResource()));

        TextView numTasksCompletedView2 = findViewById(R.id.numTasksCompletedView2);
        numTasksCompletedView2.setText(Integer.toString(statsApp2.getNumTasksCompleted()));

        TextView numTasksInterruptedView2 = findViewById(R.id.numTasksInterruptedView2);
        numTasksInterruptedView2.setText(Integer.toString(statsApp2.getNumTasksInterrupted()));

        TextView numTasksFailedView2 = findViewById(R.id.numTasksFailedView2);
        numTasksFailedView2.setText(Integer.toString(statsApp2.getNumTasksFailed()));

        TextView numTasksAbortedView2 = findViewById(R.id.numTasksAbortedView2);
        numTasksAbortedView2.setText(Integer.toString(statsApp2.getNumTasksAborted()));

        TextView cpuTimeView2 = findViewById(R.id.cpuTimeView2);
        cpuTimeView2.setText(numberFormatter.format(statsApp2.getCpuTimeUs()));

        TextView dspTimeView2 = findViewById(R.id.dspTimeView2);
        dspTimeView2.setText(numberFormatter.format(statsApp2.getDspTimeUs()));

        TextView fastrpcAvgTimeView2 = findViewById(R.id.fastrpcAvgTimeView2);
        fastrpcAvgTimeView2.setText(numberFormatter.format((int)statsApp2.getFastrpcOverheadPerCall()));

    }

    public void mainMenu(View view) {
        Intent intent = new Intent(this, MainActivity.class);
       startActivity(intent);
    }
}

/*==============================================================================
  Copyright (c) 2021,2022 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/

package com.example.android_app;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import com.example.android_app.SessionConfigs.TaskTrigger;
import android.util.Log;

public class ConfigsActivity extends AppCompatActivity {

    private static final String TAG = "AA.Configs";

    private SessionConfigs sessionConfigs;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.settings);

        // Get the Intent that started this activity and extract the string
        Intent intent = getIntent();
        Bundle configsBundle = intent.getBundleExtra(SessionConfigs.KEY_CONFIGS);
        sessionConfigs= new SessionConfigs(configsBundle);


        Log.i(TAG,"onCreate ConfigsActivity - onOff is " + sessionConfigs.isOff());

        /* Refresh settings based on current values available */
        TextView title = findViewById(R.id.titleApplicationSettings);
        title.setText("Application " + sessionConfigs.getSessionId());

        EditText vtcmSizeText = findViewById(R.id.vtcmSizeText);
        vtcmSizeText.setText(Integer.toString(sessionConfigs.getVtcmSizeMB()));

        CheckBox isSerializedCheckBox = findViewById(R.id.isSerializedCheckBox);
        isSerializedCheckBox.setChecked(sessionConfigs.isSerialized());

        EditText numSoftwareThreadsText = findViewById(R.id.numSoftwareThreadsText);
        numSoftwareThreadsText.setText(Integer.toString(sessionConfigs.getNumWorkers()));

        EditText numCyclesPerTaskText = findViewById(R.id.numCyclesPerTaskText);
        numCyclesPerTaskText.setText(Integer.toString(sessionConfigs.getMegaCyclesPerTask()));

        EditText triggerPeriodText = findViewById(R.id.taskPeriodText);
        triggerPeriodText.setText(Integer.toString(sessionConfigs.getTriggerPeriodMs()));

        if(sessionConfigs.getTaskTrigger()== TaskTrigger.TIMED) {
            RadioButton eventDrivenButton = (RadioButton) findViewById(R.id.periodicalTasksButton);
            eventDrivenButton.setChecked(true);
        } else if(sessionConfigs.getTaskTrigger()== TaskTrigger.QUEUED) {
            RadioButton queuedEventButton = (RadioButton) findViewById(R.id.queuedTasksButton);
            queuedEventButton.setChecked(true);
        } else {
            /* Not supported currently */
        }

        EditText numTasksText = findViewById(R.id.numTasksText);
        numTasksText.setText(Integer.toString(sessionConfigs.getNumTasks()));

        CheckBox endsAfterNTasksCheckBox = findViewById(R.id.endsAfterNTasksCheckBox);
        endsAfterNTasksCheckBox.setChecked(!sessionConfigs.isNeverEnding());

        /*
        CheckBox isAsynchronousCheckBox = findViewById(R.id.isAsynchronousCheckBox);
        isAsynchronousCheckBox.setChecked(sessionConfigs.isAsynchronous());
        */

        EditText priority = findViewById(R.id.threadPriorityText);
        priority.setText(Integer.toString(sessionConfigs.getThreadPriority()));

    }

/*

Don't bother with this for now but cleaner would be to grey out some fields depending on which options are selected

    public void selectPeriodicalTasks(View view) {

        EditText triggerPeriodText = findViewById(R.id.taskPeriodText);
        triggerPeriodText.setEnabled(true);
    }

    public void selectQueuedTasks(View view) {
        EditText triggerPeriodText = findViewById(R.id.taskPeriodText);
        triggerPeriodText.setEnabled(false);
    }
*/

    /** Called to return to main screen */
    public void saveSettings(View view) {
        Intent intent = new Intent(this, MainActivity.class);

        // Set bundle properties with settings values

        RadioButton periodicalTasksButton = (RadioButton) findViewById(R.id.periodicalTasksButton);
        RadioButton queuedTasksButton = (RadioButton) findViewById(R.id.queuedTasksButton);
        TaskTrigger taskTrigger= periodicalTasksButton.isChecked()? TaskTrigger.TIMED:(queuedTasksButton.isChecked()? TaskTrigger.QUEUED: TaskTrigger.CAMERA);
        sessionConfigs.setTaskTrigger(taskTrigger);

        EditText vtcmSizeText = findViewById(R.id.vtcmSizeText);
        int vtcmSize = Integer.valueOf(vtcmSizeText.getText().toString()).intValue();
        sessionConfigs.setVtcmSizeMB(vtcmSize);

        CheckBox isSerializedCheckBox = findViewById(R.id.isSerializedCheckBox);
        sessionConfigs.setSerialized(isSerializedCheckBox.isChecked());

        EditText numSoftwareThreadsText = findViewById(R.id.numSoftwareThreadsText);
        int numSoftwareThreads = Integer.valueOf(numSoftwareThreadsText.getText().toString());
        sessionConfigs.setNumWorkers(numSoftwareThreads);

        EditText numCyclesPerTaskText = findViewById(R.id.numCyclesPerTaskText);
        int numCyclesPerTask = Integer.valueOf(numCyclesPerTaskText.getText().toString());
        sessionConfigs.setMegaCyclesPerTask(numCyclesPerTask);

        EditText taskPeriodText = findViewById(R.id.taskPeriodText);
        int triggerPeriod = Integer.valueOf(taskPeriodText.getText().toString());
        sessionConfigs.setTriggerPeriodMs(triggerPeriod);

        CheckBox endsAfterNTasksCheckBox = findViewById(R.id.endsAfterNTasksCheckBox);
        sessionConfigs.setNeverEnding(!endsAfterNTasksCheckBox.isChecked());

        EditText numTasksText = findViewById(R.id.numTasksText);
        int numTasks = Integer.valueOf(numTasksText.getText().toString());
        sessionConfigs.setNumTasks(numTasks);

        EditText threadPriorityText = findViewById(R.id.threadPriorityText);
        int threadPriority  = Integer.valueOf(threadPriorityText.getText().toString());
        sessionConfigs.setThreadPriority(threadPriority);

        /*
        CheckBox isAsynchronousCheckBox = findViewById(R.id.isAsynchronousCheckBox);
        sessionConfigs.setAsynchronous(isAsynchronousCheckBox.isChecked());
        */

        intent.putExtra(SessionConfigs.KEY_CONFIGS, sessionConfigs.getBundle());
        startActivity(intent);
    }

}

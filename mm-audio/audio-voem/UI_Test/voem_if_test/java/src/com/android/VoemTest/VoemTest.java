/******************************************************************************
 * @file    VoemTest.java
 * @brief   Provides the preferences for various post processing test features
 *          that can be either enabled or disabled.
 *
 * ---------------------------------------------------------------------------
 *  Copyright (c) 2011-2012 Qualcomm Technologies, Inc.
 *  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 *  ---------------------------------------------------------------------------
 *  ******************************************************************************/

package com.android.VoemTest;

import android.app.Dialog;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceActivity;
import android.widget.Toast;
import android.util.Log;
import android.os.SystemProperties;
import android.media.AudioSystem;
import android.telephony.TelephonyManager;
import android.telephony.PhoneStateListener;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;

public class VoemTest extends PreferenceActivity {
    /** Called when the activity is first created. */
    private native void oncrpc_start();
    private native int command(int feature,boolean flag);
    private native void oncrpc_stop();
    private CheckBoxPreference wve;
    private CheckBoxPreference rve;
    private CheckBoxPreference agc;
    private CheckBoxPreference avc;
    private CheckBoxPreference ec;
    private CheckBoxPreference pf;
    private CheckBoxPreference wnr;
    private CheckBoxPreference fens;
    private CheckBoxPreference st;
    private CheckBoxPreference drx;
    private CheckBoxPreference dtx;
    private CheckBoxPreference pbe;
    private boolean flag;
    private int success ;
    private static final String VOICE_TAG="Voem_test";
    private String target;
    private TelephonyManager mTelephony;
    private PhoneStateListener mPhoneStateListener;
    @Override
    public void onCreate(Bundle savedInstanceState) {

    	super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.preferences);
	target = SystemProperties.get("ro.board.platform");

	if(target.equals("msm7630_surf"))
	    	System.loadLibrary("voem-jni");
    }

    public void onStart() {
	super.onStart();
	if(target.equals("msm7630_surf"))
        oncrpc_start();

        wve = (CheckBoxPreference)findPreference("WVE");
        if(wve != null) {
            wve.setOnPreferenceChangeListener(new OnPreferenceChangeListener()
            {
                public boolean onPreferenceChange(Preference pref,Object obj)
                {
                    flag=wve.isChecked();
	            if (target.equals("msm8960") || target.equals("msm8974") ||
                    target.equals("msm8610") || target.equals("msm8226")) {
                        boolean value = !flag;
                        AudioSystem.setParameters("wide_voice_enable="+value);
                        return true;
	            }
                    success= command(1,flag);
                    if(success!=1) {
                        Toast.makeText(VoemTest.this,"feature not defined on this target",Toast.LENGTH_LONG).show();
                        wve.setEnabled(false);
                        Log.e(VOICE_TAG,"Undefined feature enable/disable");
                    }
                    return true;
                }
            });
        }

        rve = (CheckBoxPreference)findPreference("RVE");
        if(rve != null) {
            rve.setOnPreferenceChangeListener(new OnPreferenceChangeListener()
            {
          	public boolean onPreferenceChange(Preference pref,Object obj)
          	{
                    flag=rve.isChecked();
                    success= command(2,flag);
	            if(success!=1) {
                    Toast.makeText(VoemTest.this,"feature not defined on this target",Toast.LENGTH_LONG).show();
                    rve.setEnabled(false);
		    Log.e(VOICE_TAG,"Undefined feature enable/disable");
		    }
                    return true;
                }
            });
        }

        agc = (CheckBoxPreference)findPreference("AGC");
        if(agc != null) {
            agc.setOnPreferenceChangeListener(new OnPreferenceChangeListener()
            {
          	public boolean onPreferenceChange(Preference pref,Object obj)
          	{
                    flag=agc.isChecked();
	            success=command(3,flag);
                    if(success!=1) {
                        Toast.makeText(VoemTest.this,"feature not defined on this target",Toast.LENGTH_LONG).show();
                        agc.setEnabled(false);
		        Log.e(VOICE_TAG,"Undefined feature enable/disable");
                    }
                    return true;
                }
           });
        }

        avc=(CheckBoxPreference)findPreference("AVC");
        if(avc != null) {
            avc.setOnPreferenceChangeListener(new OnPreferenceChangeListener()
            {
                public boolean onPreferenceChange(Preference pref,Object obj)
                {
                    flag=avc.isChecked();
                    success=command(4,flag);
                    if(success!=1) {
                        Toast.makeText(VoemTest.this,"feature not defined on this target",Toast.LENGTH_LONG).show();
                        avc.setEnabled(false);
			Log.e(VOICE_TAG,"Undefined feature enable/disable");
                    }
                    return true;
                }
            });
	}

        ec=(CheckBoxPreference)findPreference("EC");
        if(ec != null) {
            ec.setOnPreferenceChangeListener(new OnPreferenceChangeListener()
            {
                public boolean onPreferenceChange(Preference pref,Object obj)
                {
                    flag=ec.isChecked();
                    success=command(5,flag);
                    if(success!=1) {
                        Toast.makeText(VoemTest.this,"feature not defined on this target",Toast.LENGTH_LONG).show();
	    	        ec.setEnabled(false);
			Log.e(VOICE_TAG,"Undefined feature enable/disable");
                    }
                    return true;
                }
            });
	}

        pf=(CheckBoxPreference)findPreference("PF");
        if(pf != null) {
            pf.setOnPreferenceChangeListener(new OnPreferenceChangeListener()
            {
                public boolean onPreferenceChange(Preference pref,Object obj)
                {
                    flag=pf.isChecked();
                    success=command(6,flag);
                    if(success!=1) {
                        Toast.makeText(VoemTest.this,"feature not defined on this target",Toast.LENGTH_LONG).show();
                        pf.setEnabled(false);
			Log.e(VOICE_TAG,"Undefined feature enable/disable");
                    }
                    return true;
                }
            });
	}

        wnr=(CheckBoxPreference)findPreference("WNR");
        if(wnr != null) {
            wnr.setOnPreferenceChangeListener(new OnPreferenceChangeListener()
            {
          	public boolean onPreferenceChange(Preference pref,Object obj)
		{
		    flag=wnr.isChecked();
                    success=command(7,flag);
		    if(success!=1) {
                        Toast.makeText(VoemTest.this,"feature not defined on this target",Toast.LENGTH_LONG).show();
                        wnr.setEnabled(false);
			Log.e(VOICE_TAG,"Undefined feature enable/disable");
                    }
                    return true;
                }
            });
        }

        fens=(CheckBoxPreference)findPreference("FENS");
        if(fens != null) {
            fens.setOnPreferenceChangeListener(new OnPreferenceChangeListener()
            {
          	public boolean onPreferenceChange(Preference pref,Object obj)
          	{
          	 	flag=fens.isChecked();
                        if (target.equals("msm8960") || target.equals("msm8974") ||
                             target.equals("msm8610") || target.equals("msm8226")) {
                             boolean value = !flag;
                             AudioSystem.setParameters("fens_enable="+value);
                             return true;
                        }
          		success=command(8,flag);
			 if(success!=1)
                        {
                          Toast.makeText(VoemTest.this,"feature not defined on this target",Toast.LENGTH_LONG).show();
			  fens.setEnabled(false);
			  Log.e(VOICE_TAG,"Undefined feature enable/disable");
                        }
          		return true;
          	}
        });
	}

        st=(CheckBoxPreference)findPreference("ST");
        if(st != null) {
            st.setOnPreferenceChangeListener(new OnPreferenceChangeListener()
            {
                public boolean onPreferenceChange(Preference pref,Object obj)
                {
                    flag=st.isChecked();
                    if(target.equals("msm8960") || target.equals("msm8974") ||
                        target.equals("msm8610") || target.equals("msm8226")) {
                        boolean value = !flag;
                        AudioSystem.setParameters("st_enable="+value);
                        return true;
                    }
                    success=command(9,flag);
		    if(success!=1) {
                        Toast.makeText(VoemTest.this,"feature not defined on this target",Toast.LENGTH_LONG).show();
			st.setEnabled(false);
			Log.e(VOICE_TAG,"Undefined feature enable/disable");
                    }
                    return true;
                }
            });
	}

        drx=(CheckBoxPreference)findPreference("DRX");
        if(drx != null) {
            drx.setOnPreferenceChangeListener(new OnPreferenceChangeListener()
            {
                public boolean onPreferenceChange(Preference pref,Object obj)
                {
                    flag=drx.isChecked();
                    success=command(10,flag);
		    if(success!=1) {
                        Toast.makeText(VoemTest.this,"feature not defined on this target",Toast.LENGTH_LONG).show();
			drx.setEnabled(false);
			Log.e(VOICE_TAG,"Undefined feature enable/disable");
                    }
		    return true;
	        }
	    });
	}

	dtx=(CheckBoxPreference)findPreference("DTX");
        if(dtx != null) {
            dtx.setOnPreferenceChangeListener(new OnPreferenceChangeListener()
            {
                public boolean onPreferenceChange(Preference pref,Object obj)
                {
                    flag=dtx.isChecked();
                    success=command(11,flag);
                    if(success!=1) {
                        Toast.makeText(VoemTest.this,"feature not defined on this target",Toast.LENGTH_LONG).show();
			dtx.setEnabled(false);
			Log.e(VOICE_TAG,"Undefined feature enable/disable");
                   }
		   return true;
                }
            });
        }

	pbe=(CheckBoxPreference)findPreference("PBE");
        if(pbe != null) {
            pbe.setOnPreferenceChangeListener(new OnPreferenceChangeListener()
            {
                public boolean onPreferenceChange(Preference pref,Object obj)
                {
                    flag=pbe.isChecked();
                    success=command(12,flag);
                    if(success!=1) {
                        Toast.makeText(VoemTest.this,"feature not defined on this target",Toast.LENGTH_LONG).show();
			pbe.setEnabled(false);
			Log.e(VOICE_TAG,"Undefined feature enable/disable");
                    }
		    return true;
                }
            });
        }

        if (target.equals("msm7627_surf")) {
            if(wve != null)
                wve.setEnabled(false);
            if(rve != null)
                rve.setEnabled(false);
            if(st != null)
                st.setEnabled(false);
            if(pbe != null)
                pbe.setEnabled(false);
        }
        if (target.equals("msm8960") || target.equals("msm8974") ||
            target.equals("msm8610") || target.equals("msm8226")) {
            if(rve != null)
                rve.setEnabled(false);
            if(agc != null)
                agc.setEnabled(false);
            if(avc != null)
                avc.setEnabled(false);
            if(ec != null)
                ec.setEnabled(false);
            if(pf != null)
                pf.setEnabled(false);
            if(wnr != null)
                wnr.setEnabled(false);
            if(drx != null)
                drx.setEnabled(false);
            if(dtx != null)
                dtx.setEnabled(false);
            if(pbe != null)
                pbe.setEnabled(false);

            if( wve != null ) {
                mPhoneStateListener = new PhoneStateListener() {
	            @Override
    	            public void onCallStateChanged(int state,String incomingNumber){
                        switch(state){
	                    case TelephonyManager.CALL_STATE_IDLE:
                            wve.setEnabled(true);
   	                    break;

	                    case TelephonyManager.CALL_STATE_OFFHOOK:
	                    wve.setEnabled(false);
	                    break;

	                    case TelephonyManager.CALL_STATE_RINGING:
	                    break;
	                }
                    }
                };
   	        mTelephony = (TelephonyManager)getSystemService(Context.TELEPHONY_SERVICE);
	        if (null == mTelephony)
	            Log.e(VOICE_TAG, "Telephony service is null");
	        else
	            mTelephony.listen(mPhoneStateListener,PhoneStateListener.LISTEN_CALL_STATE);
            }
        }
    }//end of onStart()

    public void onStop()
    {
	super.onStop();
	if (target.equals("msm7630_surf"))
		oncrpc_stop();
    }
}

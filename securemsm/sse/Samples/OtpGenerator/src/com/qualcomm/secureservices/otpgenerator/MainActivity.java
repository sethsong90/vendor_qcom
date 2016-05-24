/*
 * Copyright (c) 2012-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */


package com.qualcomm.secureservices.otpgenerator;

// Required packages.
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Set;

import com.qualcomm.secureservices.otpgenerator.R;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.RadioButton;
import android.widget.TabHost;
import android.widget.TabHost.TabSpec;
import android.widget.TextView;
import android.widget.Toast;

/**
 * MainActivity Class. This class is responsible for the creation of the application.
 * An interface will be introduced and functionalities on basic UI items will be provided,
 * such as buttons, text viewers, radio buttons, dialogues.
 *
 * @see android.app.Activity
 */
public class MainActivity extends Activity {

  //-------------------------Constants-------------------------
  /** A constant of type String holds the tag for this class. */
  private static final String TAG = "OtpGenerator.MAIN_ACTIVITY";
  /** A constant of type String holds the configuration file for the application. */
  private static final String CONFIGURATION_FILE = "SSEConfiguration.txt";
  /** A constant of type String holds the salt file for the application. */
  private static final String SALT_FILE = "Salt.txt";
  /** A constant of type String holds the application's storage direction (name). */
  private static final String APP_STORAGE = "pkcs11";
  /** A constant of type String holds the token's storage name (database name). */
  private static final String TOKEN_STORAGE = "Token1.db";
  /** A constant of type String holds the key's Label. */
  private static final String KEY_LABEL = "Key1";
  /** A static final variable of type int holds the dialog error number for setting a password. */
  private static final int DIALOG_PASSWORD_SET = 0;
  /** A static final variable of type int holds the dialog error number for reseting a password. */
  private static final int DIALOG_PASSWORD_RESET = 1;
  /** A static final variable of type int holds the dialog error number for setting a salt. */
  private static final int DIALOG_SALT_SET = 2;
  /** A static final variable of type int holds the progress dialog. */
  private static final int DIALOG_PROGRESS = 3;
  /** A static final variable of type int holds the dialog shown when the application is corrupted. */
  private static final int DIALOG_CORRUPTED = 4;
  /** A static final variable of type int holds the dialog shown when help button is pressed. */
  private static final int DIALOG_HELP = 5;

  //-------------------------Global Variables-------------------------
  /** A variable of type Context holds the application's context. */
  private Context context;
  /** A variable of type Dialog holds the variable being shown to the user. */
  private Dialog dialog = null;

  /**
   * Constructor. This is an unimplemented constructor.
   */
  public MainActivity(){
  }

  static {
     System.loadLibrary("PkiOtpGenerator"); // libPkiOtpGenerator.so
  }

  private native boolean InitPKCS11  (String initStr);
  private native boolean obtainToken (String label);
  private native long createOTPKey   (String pasword, String label);
  private native long retrieveKey    (String label);
  private native boolean deleteOTPKey(String KeyLabel);
  private native long generateOTP    (long key_handle);
  private native void SetTimeBased   (boolean val);
  private native void setSalt        (String salt);
  private native String getSalt      ();
  private native boolean closePKCS11 ();

  /**
   * Method. This method is called when the application's activity is initialized (first created).
   * This is always followed by onStart() method.
   */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_activity); // Set the application's view.
        this.context = getApplicationContext(); // Set the context of the application.
        // Smooth the application's rendering.
        getWindow().setFormat(PixelFormat.RGBA_8888);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_DITHER);
        if (!initializationState()) {
          Toast toast = Toast.makeText(getApplicationContext(), "Failed to start PKCS11", Toast.LENGTH_SHORT);
          toast.show();
          finish();
          return;
        }
        this.tabHostSetup(); // Make a call to this method, to set up the tabs.
        this.addListenerOnButton(); // Make a call to this method, to set listeners to the buttons.
        this.setAboutDetails(); // Make a call to this method, to set the text for the about screen of the application.
    }

    /**
     * Called when the application's activity becomes visible to the
     * user. This method is followed by onResume() method if the
     * activity comes to the foreground, or onStop() method if it
     * becomes hidden.
     */
    public void onStart(){
      super.onStart();
      initializationState();
    }

    /**
     * Called when the system is about to start resuming a previous
     * activity. This method is followed by either onResume() if the
     * activity returns back to the front, or onStop() if it becomes
     * invisible to the user.
     */
    public void onPause(){
      super.onPause();
      closePKCS11(); // Step 5: Unload Pki library.
    }

    /**
    * Called after your activity has been stopped, prior to it being
    * started again. This is always followed by onStart() method.
    */
    public void onResume(){
      super.onResume();
      initializationState();
    }

    /**
     * Clled when the activity is no longer visible to the user,
     * because another activity has been resumed and is covering
     * this one. This method is followed by either onRestart() if
     * this activity is coming back to interact with the user, or
     * onDestroy() if this activity is going away.
     */
    public void onStop(){
      super.onStop();
      closePKCS11(); // Step 5: Unload Pki library.
    }

    /**
     * Final call received before the activity is destroyed.
     * This can happen either because the activity is finishing or
     * because the system is temporarily destroying this instance of
     * the activity to save space.
     */
    public void onDestroy(){
      super.onDestroy();
      closePKCS11(); // Step 5: Unload Pki library.
    }

    /**
     * Initialize the Pki library. The initialization will be made
     * with the written configurations given in a text file.
     */
    private boolean initializationState(){
      //-------------------------Local Variables-------------------------
      File storage = this.context.getDir(MainActivity.APP_STORAGE, MODE_PRIVATE); // A private storage for the application.
      String path = storage.getAbsolutePath(); // Save path to the application.
      String readConfiguration = this.getConfiguration(); // Call to getConfiguration will return the 'SSEConfiguration.txt' file.
      String initPki = readConfiguration + "\n" + "DB_PATH=" + path; // Create the initialization needed for the Pki object.
      //File tokenStorage = new File(path+"/"+MainActivity.TOKEN_STORAGE); // A storage for the token.

      Log.d(TAG, "-------------------------step1-------------------------");
      if (!InitPKCS11(initPki)) {
        Log.e(MainActivity.TAG, "Failed to load PKI library");
        return false;
      }
	  Log.d(TAG, "-------------------------step2-------------------------");
      if (!obtainToken("Token1")) { // Step 2: Create/Obtain the Token for the application.
        Log.e(MainActivity.TAG, "Failed to load/initialize the token");
        closePKCS11();
        return false;
      }
      return true;
    }

    /**
     * Create a tab menu in the application.
     */
    private void tabHostSetup(){

      // Hook up a TabHost with the appropriate variable.
      TabHost tabHost = (TabHost) findViewById(R.id.tabHost);
      tabHost.setup(); // Set up the TabHost.

      // Create 3 tabs to display.
        TabSpec tab1 = tabHost.newTabSpec("tab1");
        TabSpec tab2 = tabHost.newTabSpec("tab2");
        TabSpec tab3 = tabHost.newTabSpec("tab3");

        // Set the contents of the tabs.
        tab1.setContent(R.id.homeTab);
        tab2.setContent(R.id.settingsTab);
        tab3.setContent(R.id.aboutTab);

        // Set the label for the tabs.
        tab1.setIndicator("Home");
        tab2.setIndicator("Settings");
        tab3.setIndicator("About");

        // Add the tabs to the screen.
        tabHost.addTab(tab1);
        tabHost.addTab(tab2);
        tabHost.addTab(tab3);
    }

    /**
     * Hook up the buttons and set what to be executed when pressed.
     */
    public void addListenerOnButton(){
      // Hook up Buttons with the appropriate variables.
      final Button powerOnButton       = (Button) findViewById(R.id.powerOnButton);
      final Button resetPasswordButton = (Button) findViewById(R.id.resetPasswordButton);
      final Button advancedButton      = (Button) findViewById(R.id.advancedButton);
      final Button helpButton          = (Button) findViewById(R.id.helpButton);
      // Hook up textViews with the appropriate variables.
      final TextView OTPViewer  = (TextView) findViewById(R.id.OTPviewer);
      final TextView saltViewer = (TextView) findViewById(R.id.saltViewer);

      // Set a click listener to the power on button.
      powerOnButton.setOnClickListener(new OnClickListener(){
        // Set the behavior of the generator button when pressed.
        public void onClick(View v) {
          if (!powerOnButton.isSelected()){
            // If the button is not selected,
            // and if an OTP key is already created,
            if (0 != retrieveKey(MainActivity.KEY_LABEL)){
              powerOnButton.setSelected(true); // Set the button as selected.
              disableSettings(); // and disable some functions.
            } else {
              showDialog(MainActivity.DIALOG_PASSWORD_SET); // Otherwise show a dialog for setting a password.
            }
          }else{
            powerOnButton.setSelected(false); // Otherwise set the button as not selected.
            OTPViewer.setText(R.string.default_password); // Restore the default password (------).
            saltViewer.setText(""); // Restore the salt viewer to empty.
            enableSettings(); // Enable the functions.
          }
        }
      });
      // Set a click listener to the reset button.
      resetPasswordButton.setOnClickListener(new OnClickListener(){
        // Set the behavior of the reset button when pressed.
        public void onClick(View v) {
          if (0 != retrieveKey(MainActivity.KEY_LABEL)){ // If an OTP key is already created,
            showDialog(MainActivity.DIALOG_PASSWORD_RESET); // Show the password reset dialog,
          }else{
            showDialog(MainActivity.DIALOG_PASSWORD_SET); // otherwise show the password set dialog.
          }
        }
      });
      // Set a click listener to the advanced button.
      advancedButton.setOnClickListener(new OnClickListener(){
        // Set the behavior of the advanced button when pressed.
        public void onClick(View v) {
          showDialog(MainActivity.DIALOG_SALT_SET);
        }
      });
      // Set a click listener to the help button.
      helpButton.setOnClickListener(new OnClickListener(){
        // Set the behavior of the help button when pressed.
        public void onClick(View v) {
          showDialog(MainActivity.DIALOG_HELP);
        }
      });
    }

    /**
     * Set the appropriate text to the label on the About Tab.
     */
    private void setAboutDetails(){
      // Hook up a TextView with the appropriate variable.
      final TextView about = (TextView) findViewById(R.id.aboutTabViewer);
      about.setText("OTP (One-Time Password) generator\n\nVersion: 1.0\n\nCompany: Qualcomm Technologies, Inc.");
    }

    /**
     * Set the behavior of the radio buttons, when selected.
     * @param timeBasedOption the radio-button for Time Based option.
     * @param counterBasedOption the radio-button for Counter Based option.
     */
    private void radioButtonOption(RadioButton timeBasedOption, RadioButton counterBasedOption){
      if (timeBasedOption.isChecked()){ // If Time Based radio button is checked,
        SetTimeBased ( true );
      } else if (counterBasedOption.isChecked()){ // Otherwise if Counter Based radio button is checked.
        SetTimeBased ( false );
      }
    }

    /**
     * Set the behavior of the label when clicked.
     * @param view the application's view to be used.
     */
    public void labelClicked(View view){
      //Hook up a Button with the appropriate variable.
      final Button generatorButton = (Button) findViewById(R.id.powerOnButton);
      // If the generator button is selected,
      if (generatorButton.isSelected()){
        new ShowOTP().execute(""); // call the ShowOTP inner class.
      }
    }

    /**
     * Run a second thread for the application, to perform the
     * generation of an OTP.
     */
    private class ShowOTP extends AsyncTask<String, Void, String> {

      /**
       * Call the generateOTP after retrieving a key for it.
       * @return a six digit generated OTP.
       * @return null if the Pki instance object is not initialized.
       */
      @Override
      protected String doInBackground(String... arg0) {
        return String.format("%06d", generateOTP(retrieveKey(MainActivity.KEY_LABEL)));
      }

      /**
       * Executed before the execution of the thread. It displays the
       * default password (------) to the OTP monitor and, when
       * pressed, it shows the progress dialog.
       */
      @Override
      protected void onPreExecute(){
        // Hook up textViews with the appropriate variables.
        final TextView OTPViewer = (TextView) findViewById(R.id.OTPviewer);
        OTPViewer.setText(R.string.default_password);
        OTPViewer.setClickable(false);
        showDialog(MainActivity.DIALOG_PROGRESS);
      }

      /**
       * Executed after the execution of the thread.
       * It displays the generated OTP to the OTP monitor. When the
       * operation is finished, the progress dialog disappears.
       */
      @Override
      protected void onPostExecute(String result){
        // Hook up textViews with the appropriate variables.
        final TextView OTPViewer = (TextView) findViewById(R.id.OTPviewer);
        final TextView saltViewer = (TextView) findViewById(R.id.saltViewer);
        dismissDialog(MainActivity.DIALOG_PROGRESS);

        // An error occurred during OTP generation,
        if (result.equals("-00001")){
            showDialog(MainActivity.DIALOG_CORRUPTED); // show a corrupted dialog.
          } else {
            OTPViewer.setText(result); // Display the OTP,
            OTPViewer.setClickable(true);
            saltViewer.setText("Salt used: "+ getSalt()); // display the salt being used.
          }
      }

      @Override
      protected void onProgressUpdate(Void... values){

      }
    }

    /**
     * Display a dialog according to the given id number.
     * @param id The number of the dialog to be shown.
     * @return The generated dialog.
     */
    protected Dialog onCreateDialog(int id){

      switch (id){
      // The first dialog, to set the password.
      case MainActivity.DIALOG_PASSWORD_SET:

        dialog = new Dialog(this);
          // Create a new dialog as a warning if the user has not set a password.
        dialog.setContentView(R.layout.set_password_dialog); // The layout dialog is custom included in the layout folder.
        // Set the dialog's properties
        dialog.setTitle("Unset Password");
        dialog.setCanceledOnTouchOutside(false);

          // Hook up the buttons, the textView, the editText and the Image with the appropriate variables.
        TextView text = (TextView) dialog.findViewById(R.id.dialog_text);
        ImageView image = (ImageView) dialog.findViewById(R.id.dialog_image);
        Button possitiveButton = (Button) dialog.findViewById(R.id.set_positive_button);
        Button negativeButton = (Button) dialog.findViewById(R.id.set_negative_button);
        text.setText(R.string.requiredPasswordWarning); // Set the text.
        image.setImageResource(R.drawable.warning_sign); // Set the image.

        // Customising the dialog's buttons.
        possitiveButton.setText("Set");
        possitiveButton.setOnClickListener(new OnClickListener(){

          /**
           * Inner Method. This method is responsible to set the behavior of the possitive 'Set' button when pressed.
           */
          public void onClick(View v) {
            // Hook up the radiobuttons, the editText with the appropriate variables.
            RadioButton timeBased = (RadioButton) dialog.findViewById(R.id.timeModeButton);
            RadioButton counterBased = (RadioButton) dialog.findViewById(R.id.counterModeButton);
            EditText passwordInput = (EditText) dialog.findViewById(R.id.dialog_password_input);
            if (passwordInput.getText().length() != 0){ // The given password has to have a value.
              Toast toast = Toast.makeText(context, "Password set!", Toast.LENGTH_SHORT);
              toast.setGravity(Gravity.CENTER, 0, 0);
              toast.show();
              radioButtonOption(timeBased, counterBased); // Call this method to determine which is pressed.
              removeDialog(MainActivity.DIALOG_PASSWORD_SET);
              initOTPGenerator(passwordInput.getText().toString()); // Call the initOTPGenerator method.
            }
          }
        });

        negativeButton.setText("Cancel");
        negativeButton.setOnClickListener(new OnClickListener(){
          /**
           * Inner Method. This method is responsible to set the behavior of the negative 'Cancel' button when pressed.
           */
          public void onClick(View v) {
            removeDialog(MainActivity.DIALOG_PASSWORD_SET);
          }
        });
        dialog.show();
      break;

      // The dialog for resetting the password.
      case MainActivity.DIALOG_PASSWORD_RESET:

        dialog = new Dialog(this);
        // Create a new dialog for when the user wants to change his password.
        dialog.setContentView(R.layout.reset_password_dialog); // The layout dialog is custom included in the layout folder.
        // Set the dialog's properties
        dialog.setTitle("Reset Password");
        dialog.setCancelable(false);

        // Hook up Buttons, TextView, EditText and the Image with the appropriate variables.
        final Button powerOnButton1   = (Button)   findViewById(R.id.powerOnButton);
        final TextView OTPViewer1     = (TextView) findViewById(R.id.OTPviewer);
        TextView resetText            = (TextView) dialog.findViewById(R.id.dialog_text);
        ImageView resetImage          = (ImageView)dialog.findViewById(R.id.dialog_image);
        Button resetPossitiveButton   = (Button)   dialog.findViewById(R.id.reset_positive_button);
        Button resetNegativeButton    = (Button)   dialog.findViewById(R.id.reset_negative_button);
        resetText.setText(R.string.resetPasswordWarning); // Set the text.
        resetImage.setImageResource(R.drawable.warning_sign); // Set the image.

        // Customising the dialog's buttons.
        resetPossitiveButton.setText("Reset");
        resetPossitiveButton.setOnClickListener(new OnClickListener(){

          /**
           * Inner Method. This method is responsible to set the behavior of the possitive 'Reset' button when pressed.
           */
          public void onClick(View v) {
            // Hook up the radiobuttons, the editText with the appropriate variables.
            RadioButton resetTimeBased    = (RadioButton) dialog.findViewById(R.id.timeModeButton);
            RadioButton resetCounterBased = (RadioButton) dialog.findViewById(R.id.counterModeButton);
            EditText resetPasswordInput   = (EditText) dialog.findViewById(R.id.dialog_password_input);

            if (resetPasswordInput.getText().length() != 0){ // The given password has to have a value.
              Toast toast = Toast.makeText(context, "Password reset!", Toast.LENGTH_SHORT);
              toast.setGravity(Gravity.CENTER, 0, 0);
              toast.show();
              radioButtonOption(resetTimeBased, resetCounterBased);
              removeDialog(MainActivity.DIALOG_PASSWORD_RESET); // Call this method to determine which is pressed.
              resetOTPGenerator(resetPasswordInput.getText().toString()); // Call the resetOTPGenerator method.
              OTPViewer1.setClickable(true);
              resetPasswordInput.setText("");
            }
          }
        });

        resetNegativeButton.setText("Cancel");
        resetNegativeButton.setOnClickListener(new OnClickListener(){

          /**
            * Inner Method. This method is responsible to set the behavior of the negative 'Cancel' button when pressed.
            */
          public void onClick(View v) {
            removeDialog(MainActivity.DIALOG_PASSWORD_RESET);
              powerOnButton1.setSelected(false);
          }
        });

        dialog.show();
      break;

      // The dialog to set the salt.
      case MainActivity.DIALOG_SALT_SET:

        dialog = new Dialog(this);
          // Create a new dialog for when the user wants to change the salt value.
        dialog.setContentView(R.layout.set_salt_dialog); // The layout dialog is custom included in the layout folder.
        // Set the dialog's properties
        dialog.setTitle("Set Salt");
        dialog.setCanceledOnTouchOutside(false);

          // Hook up the buttons, the textView, the editText and the Image with the appropriate variables.
        TextView saltText = (TextView) dialog.findViewById(R.id.dialog_text);
        ImageView saltImage = (ImageView) dialog.findViewById(R.id.dialog_image);
        Button saltPossitiveButton = (Button) dialog.findViewById(R.id.salt_positive_button);
        Button saltNegativeButton = (Button) dialog.findViewById(R.id.salt_negative_button);
        saltText.setText(R.string.setSaltWarning); // Set the text.
        saltImage.setImageResource(R.drawable.warning_sign); // Set the image.

        // Customising the dialog's buttons.
        saltPossitiveButton.setText("Set");
        saltPossitiveButton.setOnClickListener(new OnClickListener(){

          /**
            * Inner Method. This method is responsible to set the behavior of the possitive 'Set' button when pressed.
            */
          public void onClick(View v) {
            // Hook up the EditText with the appropriate variable.
            EditText saltInput = (EditText) dialog.findViewById(R.id.dialog_salt_input);
            Toast toast = Toast.makeText(context, "Salt set!", Toast.LENGTH_SHORT);
            toast.setGravity(Gravity.CENTER, 0, 0);
            toast.show();
            removeDialog(MainActivity.DIALOG_SALT_SET);
            setSalt(saltInput.getText().toString());
            writeTextFile(saltInput.getText().toString()); // Set the salt value by saving it to a file.
          }

        });

        saltNegativeButton.setText("Cancel");
        saltNegativeButton.setOnClickListener(new OnClickListener(){

          /**
            * Inner Method. This method is responsible to set the behavior of the negative 'Cancel' button when pressed.
            */
          public void onClick(View v) {
            removeDialog(MainActivity.DIALOG_SALT_SET);
          }
        });

        dialog.show();
      break;

      // The dialog showing that progress is made to the application.
      case MainActivity.DIALOG_PROGRESS:

          // Create a new dialog for when the application generates an OTP.
        dialog = new Dialog(this, android.R.style.Theme_Translucent);
        // Set the dialog's properties
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
        dialog.setContentView(R.layout.progress_bar_dialog);
        dialog.setCancelable(false);

        dialog.show();
      break;

      // The dialog showing an error to the application (corrupted data).
      case MainActivity.DIALOG_CORRUPTED:

        dialog = new Dialog(this);
        // Create a new dialog as a warning if the application returned '-1' (error).
        dialog.setContentView(R.layout.corrupted_application_dialog); // The layout dialog is custom included in the layout folder.
        // Set the dialog's properties
        dialog.setTitle("Corrupted Application");
        dialog.setCancelable(false);

        // Hook up Buttons, TextView, Image with the appropriate variables.
        final Button powerOnButton2     = (Button) findViewById(R.id.powerOnButton);
        final Button resetPassword2     = (Button) findViewById(R.id.resetPasswordButton);
        TextView corruptedText          = (TextView) dialog.findViewById(R.id.dialog_text);
        ImageView corruptedImage        = (ImageView) dialog.findViewById(R.id.dialog_image);
        Button corruptedPossitiveButton = (Button) dialog.findViewById(R.id.corrupted_positive_button);
        Button corruptedNegativeButton  = (Button) dialog.findViewById(R.id.corrupted_negative_button);
        corruptedText.setText(R.string.corruptedApplicationWarning); // Set the text.
        corruptedImage.setImageResource(R.drawable.corrupted_sign); // Set the image.

        // Customising the dialog's buttons.
        corruptedPossitiveButton.setText("Reset");
        corruptedPossitiveButton.setOnClickListener(new OnClickListener(){
          /**
            * Inner Method. This method is responsible to set the behavior of the possitive 'Reset' button when pressed.
            */
          public void onClick(View v) {
            dismissDialog(MainActivity.DIALOG_CORRUPTED);
            showDialog(MainActivity.DIALOG_PASSWORD_RESET); // show the dialog to set the password.
          }
        });

        corruptedNegativeButton.setText("Dismiss");
        corruptedNegativeButton.setOnClickListener(new OnClickListener(){

          /**
            * Inner Method. This method is responsible to set the behavior of the negative 'Dismiss' button when pressed.
            */
          public void onClick(View v) {
            removeDialog(MainActivity.DIALOG_CORRUPTED);
            powerOnButton2.setSelected(false); // Power off.
            enableSettings(); // Enable the settings previously disabled.
            }
        });

        dialog.show();
      break;

      // The dialog showing help for the application.
      case MainActivity.DIALOG_HELP:

        dialog = new Dialog(this);
          // Create a new dialog as a warning if the user has not set a password.
        dialog.setContentView(R.layout.help_dialog); // The layout dialog is custom included in the layout folder.
        // Set the dialog's properties
        dialog.setTitle("HELP");
        dialog.setCanceledOnTouchOutside(true);
        dialog.setCancelable(true);

        dialog.show();
      break;

      }

      return dialog;
    }

    /**
     * Enable settings for when the Power On button is switched off.
     * Those settings are enabled because when the Power On button
     * is off, a password and a salt can be changed when the key
     * used before (if existing) will be deleted and substituted
     * with a new one.
     * In addition, when the Power On button is off, the label is
     * disabled to simulate the Power off.
     */
    private void enableSettings(){
      // Hook up Buttons, TextView with the appropriate variables.
      final TextView OTPViewer = (TextView) findViewById(R.id.OTPviewer);
      final Button resetPassword = (Button) findViewById(R.id.resetPasswordButton);
      final Button advancedButton = (Button) findViewById(R.id.advancedButton);

      OTPViewer.setEnabled(false); // Label is disabled (change of color).
      OTPViewer.setClickable(false); // and cannot be clicked.
      resetPassword.setEnabled(true); // Password can be changed, previous key will be deleted.
      advancedButton.setEnabled(true); // Salt can be changed, previous key will be deleted.
    }

    /**
     * Disable settings for when the Power On button is switched on.
     * Those settings are disabled because when the Power On button
     * is on, a key for the OTP is generated.
     * When a key is generated, password or salt cannot be changed.
     * In addition, when the Power On button is on, the label acts
     * like a button to generate a new OTP.
     */
    private void disableSettings(){
      // Hook up Buttons, TextView with the appropriate variables.
      final TextView OTPViewer    = (TextView) findViewById(R.id.OTPviewer);
      final Button resetPassword  = (Button) findViewById(R.id.resetPasswordButton);
      final Button advancedButton = (Button) findViewById(R.id.advancedButton);

      OTPViewer.setEnabled(true); // Label is enabled (change of color),
      OTPViewer.setClickable(true); // and can be clicked.
      resetPassword.setEnabled(false); // Cannot change password since it is already associated with the key.
      advancedButton.setEnabled(false); // Cannot change salt since it is already associated with the key.
    }

    /**
     * Call the fundamental methods of Pki class to generate an OTP
     * for the user.
     * @param userPassword the given password to be used for
     *                     generating the OTP
     */
    private void initOTPGenerator(String userPassword){
      if (!initializationState()) {
        Log.e(MainActivity.TAG,"Failed to initialize the library!");
        return;
      }
	  createOTPKey(userPassword, MainActivity.KEY_LABEL); // Step 3: Creating an OTP key.
    }

    /**
     * Call the fundamental methods of Pki class to regenerate an
     * OTP for the user.
     * @param userPassword the new given password to be used for
     *                     regenerating the OTP
     */
    private void resetOTPGenerator(String userPassword){
      if (!initializationState()) {
        Log.e(MainActivity.TAG,"Failed to initialize the library!");
        return;
      }
      deleteOTPKey(MainActivity.KEY_LABEL); // Delete the old key.
      createOTPKey(userPassword, MainActivity.KEY_LABEL); // Step 3: Recreating an OTP key.
    }

    /**
     * Read the configuration file from the asset folder and the
     * salt file from the files folder of the application.
     * @return Configuration, as a string
     */
    private String getConfiguration() {

      String configuration; // Local variable holds what is read from the configuration file found in assets folder.

      AssetManager assetManager = getAssets();
      InputStream inputStream1;
      InputStream inputStream2;

      try {
        // Read configuration file.
        inputStream1 = assetManager.open(MainActivity.CONFIGURATION_FILE,
            AssetManager.ACCESS_BUFFER);
        if (inputStream1 == null) {
          Log.e(MainActivity.TAG, "Failed to open configuration file!");
          return null;
        }
        // Check whether a files folder exists and whether a salt file exists.
        if ((context.getFilesDir() != null) && context.getFileStreamPath(SALT_FILE).exists()){
          inputStream2 = openFileInput(MainActivity.SALT_FILE); // Open the salt file.
        } else {
          writeTextFile(""); // Otherwise, create a files folder with a salt file.
          inputStream2 = openFileInput(MainActivity.SALT_FILE); // Open the salt file.
        }
      }

      catch (IOException ex) {
        Log.e(MainActivity.TAG, ex.getMessage());
        return null;
      }

      // Read both files.
      configuration = readTextFile(inputStream1);
      if (inputStream2 != null) {
        String salt = readTextFile(inputStream2);
        setSalt(salt); // Set the salt of the Pki class.
      }

      // Close both files.
      try {

        inputStream1.close();
      } catch (IOException ex) {
        Log.e(MainActivity.TAG, ex.getMessage());
      }
      try {
      if (inputStream2 != null)
        inputStream2.close();
      } catch (IOException ex) {
        Log.e(MainActivity.TAG, ex.getMessage());
      }


      return configuration;
    }

    /**
     * Read a text file.
     * @param inputStream Stream to read from
     * @return Data read from the file
     */
    private String readTextFile(InputStream inputStream) {

      //--- local variables ---
      ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
      byte buf[] = new byte[1024];
      int length;

      try {

        while ((length = inputStream.read(buf)) != -1) {
          outputStream.write(buf, 0, length);
        }

        outputStream.close();
        inputStream.close();
      }

      catch (IOException ex) {
        Log.e(MainActivity.TAG, "Error readying the file! \nDetails: "+ex.getMessage());
      }

      return outputStream.toString();
    }

    /**
     * Write the sale value to a file. If the file does not exist,
     * it will created. If it does exist, it will be replaced with a
     * new one.
     * @param salt the salt value.
     */
    private void writeTextFile(String salt){

      FileOutputStream saltOutput;

      try {
        saltOutput = openFileOutput(MainActivity.SALT_FILE, context.MODE_PRIVATE);
      } catch (FileNotFoundException e) {
        // TODO Auto-generated catch block
      Log.e(MainActivity.TAG,"Failed to create Salt file!");
        e.printStackTrace();
        return;
      }
      try {
        Log.e(MainActivity.TAG,"writing to file");
        saltOutput.write(salt.getBytes());
        Log.e(MainActivity.TAG,"setting in PKI");
        setSalt(salt); // Set the salt of the Pki class.
      } catch (IOException e) {
        // TODO Auto-generated catch block
        e.printStackTrace();
      }
      try {
        saltOutput.close();
      } catch (IOException e) {
        // TODO Auto-generated catch block
        e.printStackTrace();
      }
    }

} // End of class.

package org.srb2;

import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.SurfaceHolder.Callback;
import android.app.Activity;
import android.os.Bundle;

public class SRB2Game extends Activity implements Callback {
	public static String TAG = "SRB2-Activity";
	private SurfaceView sv;
	private GameThread thread;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.main);

		sv = (SurfaceView) findViewById(R.id.SoftwareRendererDisplay);
		sv.getHolder().addCallback(this);
	}

	public void surfaceChanged(SurfaceHolder arg0, int arg1, int arg2, int arg3) {
		Log.e(TAG, "Output surface changed? OHSHI-");
	}

	public void surfaceCreated(SurfaceHolder arg0) {
		Log.d(TAG, "Output surface ready!  Instantiating and starting game...");
		thread = new GameThread(sv.getHolder());
		thread.start();
	}

	public void surfaceDestroyed(SurfaceHolder arg0) {
		// TODO shutdown SRB2 as cleanly as possible.
	}
}

package org.srb2;

import org.srb2.nativecode.SRB2;

import android.graphics.Canvas;
import android.util.Log;
import android.view.SurfaceHolder;

public class GameThread extends Thread {
	public static String TAG = "SRB2-GameThread";
	private SurfaceHolder sh;
	private SRB2 srb2;

	public GameThread(SurfaceHolder h) {
		super();
		this.srb2 = new SRB2(h);
		this.sh = h;
	}

	@Override
	public void run() {
		Log.d(TAG, "Starting thread!");
		this.srb2.run();
	}
}

package org.srb2.nativecode;

import android.util.Log;
import android.view.SurfaceHolder;

/// Wraps the entire native game.  This object should be wholly owned
/// by the thread it's going to run in.
public class SRB2 {
	public static String TAG = "SRB2-Wrapper";
	private Main main;
	public Video video;

	public SRB2(SurfaceHolder videoOut) {
		try {
            Log.i(TAG, "Loading native SRB2 shared object from package...");
            System.load("/data/data/org.srb2/lib/libsrb2.so");

        } catch (UnsatisfiedLinkError ule) {
            Log.i(TAG, "... it doesn't appear to be installed in the package.  Looking for native library in the global search path.");
            try {
                System.load("libsrb2.so");

            } catch (UnsatisfiedLinkError ule2) {
                Log.e("JNI", "... no luck.  Could not load libsrb2.so!");
                return;
            }
        }
        this.video = new Video(this, videoOut);
		this.main = new Main(this);
	}

	public void run() {
		this.main.main(this.video);
	}
}
